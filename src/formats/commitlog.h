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

#ifndef RCOMMIT_LOG_H
#define RCOMMIT_LOG_H


#include "../core/seeklog.h"
#include "../core/display.h"
#include "../core/regex.h"
#include "../core/stringhash.h"

class ILogMill;

#include <time.h>
#include <string>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <functional>
#include <optional>
#include "sys/stat.h"

class RCommitFile {
public:
    std::string filename;
    std::string action;
    vec3 colour;

    RCommitFile(const std::string& filename, const std::string& action, vec3 colour);
};

class RCommit {
    vec3 fileColour(const std::string& filename);
public:
    time_t timestamp;
    std::string username;

    std::list<RCommitFile> files;

    void postprocess();
    bool isValid();

    void addFile(const std::string& filename, const std::string& action);
    void addFile(const std::string& filename, const std::string& action, const vec3& colour);

    RCommit();
    void debug();
    virtual bool parse(BaseLog* logf) { return false; };
};

class ICommitLog {
public:
    virtual ~ICommitLog(){}

    static std::string filter_utf8(const std::string& str);
    virtual void seekTo(float percent) = 0;
    virtual bool checkFormat() = 0;
    virtual std::string getLogCommand() = 0;
    static int systemCommand(const std::string& command);
    virtual void requireExecutable(const std::string& exename) = 0;
    virtual void bufferCommit(RCommit& commit) = 0;
    virtual bool getCommitAt(float percent, RCommit& commit) = 0;
    virtual bool findNextCommit(RCommit& commit, int attempts) = 0;
    virtual bool nextCommit(RCommit& commit, bool validate = true) = 0;
    virtual bool hasBufferedCommit() = 0;
    virtual bool isFinished() = 0;
    virtual bool isSeekable() = 0;
    virtual float getPercent() = 0;
};



class RCommitLog : public ICommitLog {
protected:
    BaseLog* logf;

    std::string temp_file;
    std::string log_command;

    std::string lastline;

    bool is_dir;
    bool success;
    bool seekable;

    RCommit lastCommit;
    bool buffered;

    bool checkFirstChar(int firstChar, std::istream& stream);

    bool createTempLog();
    static bool createTempFile(std::string& temp_file);

    bool getNextLine(std::string& line);

    virtual bool parseCommit(RCommit& commit) { return false; };
public:
    RCommitLog(const std::string& logfile, int firstChar = -1);
    virtual ~RCommitLog();


    virtual void seekTo(float percent);

    virtual bool checkFormat();

    virtual std::string getLogCommand();

    virtual void requireExecutable(const std::string& exename);

    virtual void bufferCommit(RCommit& commit);

    virtual bool getCommitAt(float percent, RCommit& commit);
    virtual bool findNextCommit(RCommit& commit, int attempts);
    virtual bool nextCommit(RCommit& commit, bool validate = true);
    virtual bool hasBufferedCommit();
    virtual bool isFinished();
    virtual bool isSeekable();
    virtual float getPercent();
};

class MultiCommitLog : public ICommitLog {

    std::multimap<time_t, RCommit> bufferedCommits;

    virtual bool parseCommit(RCommit& commit) { return false; };

    std::multimap<time_t, RCommit> commits;
public:
    struct logPack {
        ICommitLog *log;
        std::string base;
        std::optional<RCommit> lastCommit;
    };
private:
    std::priority_queue<logPack, std::vector<logPack> , std::greater<logPack> > queue;
public:



    MultiCommitLog(const std::vector<std::unique_ptr<ILogMill>> &mills);
    virtual ~MultiCommitLog();


    virtual void seekTo(float percent);

    virtual bool checkFormat();

    virtual std::string getLogCommand();

    virtual void requireExecutable(const std::string& exename);

    virtual void bufferCommit(RCommit& commit);

    virtual bool getCommitAt(float percent, RCommit& commit);
    virtual bool findNextCommit(RCommit& commit, int attempts);
    virtual bool nextCommit(RCommit& commit, bool validate = true);
    virtual bool hasBufferedCommit();
    virtual bool isFinished();
    virtual bool isSeekable();
    virtual float getPercent();
};


#if 0
inline bool operator<(const MultiCommitLog::logPack &a, const MultiCommitLog::logPack &b) {
    if( a.lastCommit && ! b.lastCommit) return false;
    if( a.lastCommit && a.lastCommit.value().timestamp >= b.lastCommit.value().timestamp ) return false;
    return a.base < b.base;
}
#endif

inline bool operator>(const MultiCommitLog::logPack &a, const MultiCommitLog::logPack &b) {
    time_t at =0, bt=0;
    if( a.lastCommit ) at = a.lastCommit.value().timestamp;
    if( b.lastCommit ) bt = b.lastCommit.value().timestamp;

    if( at > bt ) return true;
    if( at < bt ) return false;
    return a.base > b.base;
}

#endif
