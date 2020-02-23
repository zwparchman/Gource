#include "Timing.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <atomic>
#include <sstream>
#include <iomanip>



TimerWriter::Updater::Updater( TimerWriter &parrent): parrent(parrent) {
    start = parrent.clock.now();
}
TimerWriter::Updater::~Updater(){
    parrent.ranFor += parrent.clock.now() - start;
    parrent.calls ++;
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
        std::ofstream ofile(fileName, std::ios_base::app );

        std::stringstream ss;
        ss << std::setw(15) << ticks <<": calls: " << std::setw(11) << calls << ": " <<  heading;

        ofile << ss.str() << std::endl;
        std::cout << ss.str() <<std::endl;
        ofile.close();

    } else {
        fprintf(stderr, "Could not open file <%s> for TimerWriter with heading <%s>",
                fileName.data(), heading.data());
    }
}
