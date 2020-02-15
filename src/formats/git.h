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

struct GitRepo {
    std::shared_ptr<git_repository*> ptr;

    GitRepo(){};

    GitRepo(char const * location) :
        ptr(new git_repository*, [](auto* g){ if( g ) git_repository_free(*g);delete g;})
    {
        if(git_repository_open(&*ptr, location))
        {
            ptr = nullptr;
        }
    }
};

struct GitRevWalker {
    std::shared_ptr<git_revwalk*> ptr;
    GitRepo repo;

    GitRevWalker(){};

    GitRevWalker(GitRepo &repo) :
        repo(repo),
        ptr(new git_revwalk*, [](auto* g){ if( g ) git_revwalk_free(*g);delete g;})
    {
        if(git_revwalk_new(&*ptr, *repo.ptr) )
        {
            ptr = nullptr;
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

class GitCommitLog : public RCommitLog {
protected:
    bool parseCommit(RCommit& commit);
    BaseLog* generateLog(const std::string& dir);
    static void readGitVersion();
public:
    GitCommitLog(const std::string& logfile);
    GitCommitLog(const GitCommitLog&)=delete;

    
    static std::string logCommand();

    GitRepo repo;
    GitRevWalker revWalker;

    virtual ~GitCommitLog(){};
};

#endif
