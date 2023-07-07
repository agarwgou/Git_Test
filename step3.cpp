#include <iostream>

#include "CsvFeeder.h"
#include "Msg.h"
#include "VolSurfBuilder.h"
#include "CubicSmile.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << " outputFile.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];
    std::ofstream outputFile(argv[2]);
    outputFile<<"TIME,EXPIRY,FUT_PRICE,ATM,BF25,RR25,BF10,RR10,FITTING_ERROR"<<endl;;
    
    VolSurfBuilder<CubicSmile> volBuilder;
    auto feeder_listener = [&volBuilder] (const Msg& msg) {
        if (msg.isSet) {
            volBuilder.Process(msg);
        }
    };
    
    auto timer_listener = [&volBuilder,&outputFile] (uint64_t now_ms) {
        // fit smile
        auto smiles = volBuilder.FitSmiles();
        
        // TODO: stream the smiles and their fitting error to outputFile.csv
        for (auto iter = smiles.begin(); iter != smiles.end(); iter++) {
            cout<<now_ms<<" " <<iter->first<<","<<(iter->second).first.futPrice<<","<<(iter->second).first.atmvol_<<","<<(iter->second).first.bf25_<<","<<(iter->second).first.rr25_<<","<<(iter->second).first.futPrice<<","<<(iter->second).first.bf10_<<","<<(iter->second).first.rr10_<<","<<(iter->second).second<<endl;
            
            outputFile<<formatTimestamp(now_ms)<<"," <<formatDatetime(iter->first)<<","<<(iter->second).first.futPrice<<","<<(iter->second).first.atmvol_<<","<<(iter->second).first.bf25_<<","<<(iter->second).first.rr25_<<","<<(iter->second).first.bf10_<<","<<(iter->second).first.rr10_<<","<<(iter->second).second<<endl;
        }
            
    };

    const auto interval = std::chrono::minutes(1);  // we call timer_listener at 1 minute interval
    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);
    while (csv_feeder.Step()) {
    }
    
    outputFile.close();
    cout<<"Hurray! We are done reading the file and Publishing the fitted smiles. Please check "<<argv[2]<<endl;
    return 0;
    
    
}
