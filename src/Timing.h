#include <chrono>
#include <string>
#include <atomic>

class TimerWriter ;

class TimerWriter {
    std::chrono::high_resolution_clock clock;

    std::chrono::duration<float> ranFor;
    std::atomic<int> calls;
    std::string fileName;
    std::string heading;

public:
    class Updater{
    public:
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        TimerWriter &parrent;
        Updater(TimerWriter& parrent);

        ~Updater();
    };

    TimerWriter(const char * fileName, const char * heading);
    Updater getUpdater();
    ~TimerWriter();

};
