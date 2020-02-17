/*
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "git2.h"
#include "../gource_settings.h"
#include "git.h"
#include <unistd.h>

#include "../Timing.h"

// parse git log entries

//git-log command notes:
// - no single quotes on WIN32 as system call treats them differently
// - 'user:' prefix allows us to quickly tell if the log is the wrong format
//   and try a different format (eg cvs-exp)

Regex git_version_regex("([0-9]+)(?:\\.([0-9]+))?(?:\\.([0-9]+))?");

void GitCommitLog::readGitVersion() {
}

std::string GitCommitLog::logCommand() {
    return "nope";
}

static void PushCommits(
                        git_repository * repo,
                        Channel<RCommit> * chan,
                        std::atomic<bool> * fetching,
                        std::string prefix){
    prefix += "/";
    usleep(1000);
//    static TimerWriter timer("timing.txt", "git.cpp:PushCommits");
//    auto up = timer.getUpdater();
    if( ! chan ) {
        //fprintf(stderr, "PushCommits must have a Channel to communicate\n");
        return;
    }


    if( ! repo ) {
        //fprintf(stderr, "PushCommits must have a valid repo \n");
        chan->close();
        return;
    }

    git_revwalk * revWalker = nullptr;
    std::unordered_set<std::string> seen;

    while( ! chan->is_closed()) {
//        static TimerWriter timer("timing.txt", "git.cpp:PushCommits - main loop");
//        auto up = timer.getUpdater();

        RCommit ocommit;
        if( !revWalker) {
            if( git_revwalk_new(&revWalker, repo) ){
                //fprintf(stderr, "Failed to build the revWalker: %s", git_error_last()->message);
                continue;
            }
            git_revwalk_sorting(revWalker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME | GIT_SORT_REVERSE);
            if( git_revwalk_push_glob(revWalker, "*") ){
                continue;
            }
        }

        if( ! revWalker )
        {
            usleep(10000);
            continue;
        }

        git_oid nxt;
        {
//            static TimerWriter timer("timing.txt", "git.cpp:PushCommits - revwalk_next");
//            auto up = timer.getUpdater();

            if( int err = git_revwalk_next( &nxt, revWalker) ){
                if( err == GIT_ITEROVER ) {
                    //start_fetch(repo);
                    //TODO fetch repo
                    if( fetching ) {
                        *fetching = true;
                    }
                    usleep(1000000); // usleep is in microseconds
                    if( git_revwalk_push_glob(revWalker, "*") ){
                        continue;
                    }
                }
                git_revwalk_free(revWalker);
                revWalker = nullptr;
                continue;
            }
            {
//                static TimerWriter timer("timing.txt", "git.cpp:PushCommits - hide current oid");
//                auto up = timer.getUpdater();

                git_revwalk_hide(revWalker, &nxt);
            }


            {
//                static TimerWriter timer("timing.txt", "git.cpp:PushCommits - add oid to seen");
//                auto up = timer.getUpdater();

                std::string asString;
                char buffer[GIT_OID_HEXSZ+1];
                asString = git_oid_tostr(buffer, GIT_OID_HEXSZ+1, &nxt);

                if( seen.find(asString) == seen.end()){
                    seen.emplace(asString);
                } else {
                    continue;
                }
            }

        }

        git_object * obj;
        {
//            static TimerWriter timer("timing.txt", "git.cpp:PushCommits - object lookup");
//            auto up = timer.getUpdater();


            if( git_object_lookup(&obj, repo, &nxt, GIT_OBJECT_ANY) ){
                printf("Object lookup failed\n");
                git_revwalk_free(revWalker);
                revWalker = nullptr;
            }
        }

        auto type = git_object_type(obj);
        if( type == GIT_OBJ_COMMIT ) {
            git_commit * commit = (git_commit*)obj;
//            static TimerWriter timer("timing.txt", "git.cpp:PushCommits - gather commit data");
//            auto up = timer.getUpdater();

            std::vector<git_commit*> parents;
            int pcount = git_commit_parentcount(commit);
            parents.resize(  pcount );

            for(int i=0; i<(int)parents.size(); i++){
                git_commit_parent(&(parents[i]), commit, i);
            }

            const git_signature * sig = git_commit_author(commit);
            //std::cout<<"Commit by ["<<sig->name<<"] at ["<<sig->when.time<<"] parents ["<<pcount<< "]"<<std::endl;

            ocommit.username = sig->name;
            ocommit.timestamp = sig->when.time;

            git_diff *diff = nullptr;
            git_tree *ctree = nullptr, *ptree = nullptr;
            {
//                static TimerWriter timer("timing.txt", "git.cpp:PushCommits - tree gathering");
//                auto up = timer.getUpdater();

                if( git_commit_tree(&ctree, commit) ) {
                    printf("Error getting commit tree\n");
                    git_revwalk_free(revWalker);
                    revWalker = nullptr;
                }

                if( pcount >= 1 ) {
                    if( git_commit_tree(&ptree, parents[0]) ) {
                        printf("Error getting parent tree\n");
                        git_revwalk_free(revWalker);
                        revWalker = nullptr;
                    }
                }
            }


            //do the tree walk to find modified files.
            {
//                static TimerWriter timer("timing.txt", "git.cpp:PushCommits - diff tree walking");
//                auto up = timer.getUpdater();

                struct Pack {
                    RCommit *commit;
                    std::string prefix;
                } pack = {
                    &ocommit,
                    prefix,
                };
                git_diff_tree_to_tree(&diff, repo, ptree, ctree, nullptr);
                git_diff_foreach(diff, 
                                 //file cb
                                 [](const git_diff_delta *delta, float, void *payload){
                                    Pack *pack = (Pack*)payload;
                                    if( ! (delta->old_file.flags & GIT_DIFF_FLAG_EXISTS) ) {
                                        pack->commit->addFile(pack->prefix + delta->new_file.path, "A");
                                        //printf("Created [%s]\n", delta->new_file.path);
                                    } else if (  ! (delta->new_file.flags & GIT_DIFF_FLAG_EXISTS) ){
                                        pack->commit->addFile(pack->prefix + delta->new_file.path, "D");
                                        //printf("Deleted [%s]\n", delta->old_file.path);
                                    } else {
                                        pack->commit->addFile(pack->prefix + delta->new_file.path, "M");
                                        //printf("Modified [%s]\n", delta->old_file.path);
                                    }
                                    return 0;
                                 },
                                 nullptr, //bin cb
                                 nullptr, // hunk cb
                                 nullptr, // line cb
                                 (void*) &pack); // payload
            }

            git_diff_free(diff);
            git_tree_free(ctree);
            git_tree_free(ptree);

            for(int i=0; i<(int)parents.size(); i++){
                git_commit_free(parents[i]);
            }
        }
        git_object_free(obj);

        chan->put(ocommit);
    }
}

GitCommitLog::GitCommitLog(const std::string& logfile):
    logfileName(logfile)
    ,repo( logfile.c_str())
    ,commitChannel(std::make_shared<Channel<RCommit>>())
    ,commitFinder(PushCommits, *repo.ptr, &*commitChannel, &fetching, logfile)
{
    //can generate log from directory
    if( !repo.ptr ){
        finished  = true;
        return;
    }
}

void GitCommitLog::seekTo(float percent){ }
// there is no format so it can't be wrong
bool GitCommitLog::checkFormat(){ return true; }
void GitCommitLog::requireExecutable(const std::string& exename){ }
// don't buffer commits
void GitCommitLog::bufferCommit(RCommit& commit){ }
// we don't do this for now
bool GitCommitLog::getCommitAt(float percent, RCommit& commit){
    return false;
}

// there is no reatempting as we have to wait on the thread
bool GitCommitLog::findNextCommit(RCommit& commit, int /* attempts */ ){
    return nextCommit(commit);
}
bool GitCommitLog::hasBufferedCommit(){
    return commitChannel->size() > 0;
}
bool GitCommitLog::isFinished(){
    if( finished || (commitChannel->is_closed() && commitChannel->size() == 0)){
        finished = true;
        return true;
    }
    return finished;
}

bool GitCommitLog::isFetching(){
    if( fetching != wasFetching ) {
        printf("[%s] going into fetching mode [%i]\n", logfileName.c_str(), (int) fetching);
        wasFetching = fetching;
    }
    return fetching;
}

// currently not supported
bool GitCommitLog::isSeekable(){
    return false;
}

// this will change when seeking is supported
float GitCommitLog::getPercent(){
    return 0.0f;
}

bool GitCommitLog::nextCommit(RCommit &commit, bool validate ) {
    if( commitChannel->get(commit, false)){
        //fprintf(stderr, "GitCommitLog::nextCommit returning true\n");
        return true;
    } else if ( commitChannel->is_closed()) {
        //fprintf(stderr, "GitCommitLog::nextCommit is now finished\n");
        finished = true;
    }
    //fprintf(stderr, "GitCommitLog::nextCommit returning false\n");
    return false;
}

GitCommitLog::~GitCommitLog(){
    commitChannel->close();
    commitFinder.join();
}
