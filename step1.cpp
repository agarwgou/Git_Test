#include <iostream>

#include "CsvFeeder.h"
#include "Msg.h"
#include <fstream>
#include <string>
#include <iostream>


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << std::endl;
        return 1;
    }
    std::cout<<"Setting file: "<<argv[1]<<std::endl;
    const char* ticker_filename = argv[1];
    
    std::cout<<"Setting feeder listner"<<std::endl;
    auto feeder_listener = [&] (const Msg& msg) {
        if (msg.isSet) {
            std::cout << msg.timestamp << ", isSnap = " << msg.isSnap << ", numUpdates = " << msg.Updates.size() << std::endl;
        }
    };

    std::cout<<"Setting time listner"<<std::endl;
    auto timer_listener = [&] (uint64_t now_ms) {
        std::cout << "timer_listener called: " << now_ms << std::endl;
    };

    std::cout<<"Setting interval"<<std::endl;
    const auto interval = std::chrono::minutes(1);  // we call timer_listener at 1 minute interval

    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);

    std::cout<<"Running Step"<<std::endl;
    while (csv_feeder.Step()) {
    }
    return 0;
}
