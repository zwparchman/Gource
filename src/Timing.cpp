#include "Timing.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

TimerWriter::Updater::Updater( TimerWriter &parrent): parrent(parrent) {
    start = parrent.clock.now();
}
TimerWriter::Updater::~Updater(){
    parrent.ranFor += parrent.clock.now() - start;
}


TimerWriter::TimerWriter(const char * fileName, const char * heading){
    this->fileName = fileName;
    this->heading = heading;
}
TimerWriter::Updater TimerWriter::getUpdater(){
    Updater up(*this);
    return up;
}

TimerWriter::~TimerWriter(){
    FILE * file = fopen(fileName.data(), "a");
    if( file ) {
        float ticks = ranFor.count();
        std::ofstream ofile(fileName);
        ofile << heading << ": " << ticks<<std::endl;
        std::cout << heading << ": " << ticks<<std::endl;
        ofile.close();

    } else {
        fprintf(stderr, "Could not open file <%s> for TimerWriter with heading <%s>",
                fileName.data(), heading.data());
    }
}
