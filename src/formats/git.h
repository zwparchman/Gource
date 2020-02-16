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

#ifndef GITLOG_H
#define GITLOG_H

#include "commitlog.h"
#include <git2.h>
#include <memory>
#include <unordered_set>
#include "../Channel.hpp"
#include <atomic>

struct GitRepo {
    std::shared_ptr<git_repository*> ptr;

    GitRepo(){};

    GitRepo(char const * location) :
        ptr(new git_repository*, [](auto* g){ if( g && *g ) git_repository_free(*g);delete g;})
    {
        if(git_repository_open(&*ptr, location))
        {
            *ptr = nullptr;
            fprintf(stderr, "Git error %s\n", git_error_last()->message);
        }
    }
};

struct GitRevWalker {
    std::shared_ptr<git_revwalk*> ptr;
    GitRepo repo;

    GitRevWalker(){};

    GitRevWalker(GitRepo &repo) :
        repo(repo),
        ptr(new git_revwalk*, [](auto* g){ if( g && *g) git_revwalk_free(*g);delete g;})
    {
        if((!*repo.ptr) || git_revwalk_new(&*ptr, *repo.ptr) )
        {
            *ptr = nullptr;
        }
    }
};

struct GitCommit {
    std::shared_ptr<git_commit*> ptr;
    GitCommit():
        ptr(new git_commit*, [](auto* g){ if( g ) git_commit_free(*g);delete g;})
        {}

    static GitCommit Lookup(const GitRepo &repo, git_oid id)
    {
        GitCommit com;
        if( git_commit_lookup(&*com.ptr, *repo.ptr, &id) ) {
            com.ptr = nullptr;
        }

        return com;
    }
};

class GitCommitLog : public ICommitLog{
protected:
    BaseLog* generateLog(const std::string& dir);
    static void readGitVersion();

    std::shared_ptr<Channel<RCommit>> commitChannel;

    std::atomic<bool> finished = false;
    std::atomic<bool> fetching = false;

public:
    GitCommitLog(const std::string& logfile);
    GitCommitLog(const GitCommitLog&)=delete;

    // from ICommitLog
    virtual void seekTo(float percent);
    virtual bool checkFormat();
    virtual void requireExecutable(const std::string& exename);
    virtual void bufferCommit(RCommit& commit);
    virtual bool getCommitAt(float percent, RCommit& commit);
    virtual bool findNextCommit(RCommit& commit, int attempts);
    virtual bool nextCommit(RCommit& commit, bool validate = true);
    virtual bool hasBufferedCommit();
    virtual bool isFinished();
    virtual bool isSeekable();
    virtual float getPercent();
    virtual std::string getLogCommand(){ return "nope"; }
    virtual bool isFetching();

    static std::string logCommand();

    GitRepo repo;

    // std::thread< static void (*)(GitRepo , Channel<RCommit> )
    std::thread commitFinder;

    virtual ~GitCommitLog();
};

#endif
