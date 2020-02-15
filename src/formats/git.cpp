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

GitCommitLog::GitCommitLog(const std::string& logfile) : RCommitLog(logfile, 'u') {
    //can generate log from directory
    if(!logf && is_dir) {
        repo = GitRepo(logfile.c_str());
        revWalker = GitRevWalker(repo);

        if( git_revwalk_push_glob(*revWalker.ptr, "*") ){
            repo.ptr = nullptr;
            revWalker.ptr = nullptr;
        }

        git_revwalk_sorting(*revWalker.ptr, GIT_SORT_TOPOLOGICAL | GIT_SORT_REVERSE);

        if( repo.ptr ) {
            success  = true;
            seekable = false;
        }
    }
}

// parse modified git format log entries
bool GitCommitLog::parseCommit(RCommit& ocommit) {
    static TimerWriter timer("timing.txt", "GitCommitLog::parseCommit");
    auto up = timer.getUpdater();

    git_oid nxt;
    {
        static TimerWriter timer("timing.txt", "GitCommitLog::parseCommit - revwalk_next");
        auto up = timer.getUpdater();

        if( git_revwalk_next( &nxt, *revWalker.ptr) ){
            return false;
        }
    }

    git_object * obj;
    if( git_object_lookup(&obj, *repo.ptr, &nxt, GIT_OBJECT_ANY) ){
        printf("Object lookup failed\n");
        return false;
    }

    auto type = git_object_type(obj);
    git_commit * commit = (git_commit*)obj;
    if( type == GIT_OBJ_COMMIT ) {

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

        if( git_commit_tree(&ctree, commit) ) {
            printf("Error getting commit tree\n");
            return false;
        }

        if( pcount >= 1 ) {
            if( git_commit_tree(&ptree, parents[0]) ) {
                printf("Error getting parent tree\n");
                return false;
            }
        }


        //do the tree walk to find modified files.
        {
            static TimerWriter timer("timing.txt", "GitCommitLog::parseCommit - diff tree walking");
            auto up = timer.getUpdater();

            git_diff_tree_to_tree(&diff, *repo.ptr, ptree, ctree, nullptr);
            git_diff_foreach(diff, 
                             //file cb
                             [](const git_diff_delta *delta, float, void *payload){
                                RCommit *com = (RCommit*)payload;
                                if( ! (delta->old_file.flags & GIT_DIFF_FLAG_EXISTS) ) {
                                    com->addFile(delta->new_file.path, "A");
                                    //printf("Created [%s]\n", delta->new_file.path);
                                } else if (  ! (delta->new_file.flags & GIT_DIFF_FLAG_EXISTS) ){
                                    com->addFile(delta->new_file.path, "D");
                                    //printf("Deleted [%s]\n", delta->old_file.path);
                                } else {
                                    com->addFile(delta->new_file.path, "M");
                                    //printf("Modified [%s]\n", delta->old_file.path);
                                }
                                return 0;
                             },
                             nullptr, //bin cb
                             nullptr, // hunk cb
                             nullptr, // line cb
                             (void*) &ocommit); // payload
        }

        git_diff_free(diff);
        git_tree_free(ctree);
        git_tree_free(ptree);

        for(int i=0; i<(int)parents.size(); i++){
            git_commit_free(parents[i]);
        }
    }
    git_object_free(obj);

    return true;
}
