/*
    Copyright (C) 2012 Andrew Caudwell (acaudwell@gmail.com)

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

#ifndef LOGMILL_H
#define LOGMILL_H

#include <boost/filesystem.hpp>

#include "SDL_thread.h"

#include "core/sdlapp.h"
#include "core/display.h"

#include "formats/commitlog.h"

#if defined(HAVE_PTHREAD) && !defined(_WIN32)
#include <signal.h>
#endif

#include <memory>

enum {
    LOGMILL_STATE_STARTUP,
    LOGMILL_STATE_FETCHING,
    LOGMILL_STATE_SUCCESS,
    LOGMILL_STATE_FAILURE
};

struct ILogMill {
    std::string base="";
    virtual ~ILogMill(){}
    virtual void run() = 0;
    virtual void abort() = 0;
    virtual std::string getError() = 0;
    virtual int getStatus() = 0;
    virtual bool isFinished() = 0;
    virtual ICommitLog* getLog() = 0;
};

class RLogMill: public ILogMill {
    SDL_Thread* thread;
    SDL_mutex* mutex;
    SDL_cond* cond;
    
    int logmill_thread_state;

    std::string logfile;
    ICommitLog* clog;

    std::string error;

    bool findRepository(boost::filesystem::path& dir, std::string& log_format);
    ICommitLog* fetchLog(std::string& log_format);
public:
    RLogMill(const std::string& logfile);
    virtual ~RLogMill();

    virtual void run();

    virtual void abort();

    virtual std::string getError();

    virtual int getStatus();
    virtual bool isFinished();

    virtual ICommitLog* getLog();
};


class MultiLogMill: public ILogMill {
    int mill=0;
    std::vector<std::unique_ptr<ILogMill> > mills;

    std::optional<MultiCommitLog> commitLog;
public:
    virtual ~MultiLogMill();
    MultiLogMill(const std::vector<std::string> &logs);

    virtual void run();
    virtual void abort();
    virtual std::string getError();
    virtual int getStatus();
    virtual bool isFinished();
    virtual ICommitLog* getLog();
};


#endif
