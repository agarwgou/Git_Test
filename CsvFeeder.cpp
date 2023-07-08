#include <iostream>
#include "CsvFeeder.h"
#include "date/date.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
using namespace std;

//Function to convert Time String to UnixMS format
//Git test-2
uint64_t TimeToUnixMS(std::string ts) {
    std::istringstream in{ts};
    std::chrono::system_clock::time_point tp;
    in >> date::parse("%FT%T", tp);
    const auto timestamp = std::chrono::time_point_cast<std::chrono::milliseconds>(tp).time_since_epoch().count();
    return timestamp;
}


//Function to conver Time in UnixMS back to string
std::string formatTimestamp(uint64_t timestamp) {
    // Convert the timestamp to a time_point
    std::chrono::system_clock::time_point timePoint =
        std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp));

    // Convert the time_point to a time_t value
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);

    // Format the time using std::put_time and std::gmtime
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << timestamp % 1000;
    oss << "Z";

    return oss.str();
}

//Create Msg from feed data
bool ReadNextMsg(std::ifstream& file, Msg& msg) {
    if (file.eof()) {
        return false;
    }
    
    //Debug code
    //std::cout<<"ReadNxtmsg_msg_timestamppre"<<msg.timestamp<<std::endl; //Gourav
    //std::cout<<"ReadNxtmsg"<<file.is_open()<<std::endl;
    
    // TODO: your implementation to read file and create the next Msg into the variable msg
    

    std::string line;
    cout<<"Step-1: Reading Message For Next Timestamp..."<<endl;
    while (true) { //this loop runs until the timestamp changes or if it is the end of the file
        
        if (file.eof()) {
            break;
        }
        //save the current position in the file
        size_t lastIndex = file.tellg();
        
        //Read a line
        getline(file,line);
        vector<std::string> tokens;
        istringstream iss(line);
        string token;
        while (getline(iss,token,','))
        {
            tokens.push_back(token);
        }
        
        
        // Exit Loop when the time stamp changes
        if (TimeToUnixMS(tokens[1]) != msg.timestamp && msg.Updates.size()!=0){
            file.seekg(lastIndex); //revert back to previous line before breaking the loop
            cout<<"<<<<End of MESSAGE>>>>>"<<endl;
            break;
        }
        
        //Print the line which was read
        for (const auto& t : tokens)
        {
            cout<< t<< " ";
        }
        cout<<endl;
        
        //Start Populating the msg with line data
        
        msg.timestamp = TimeToUnixMS(tokens[1]);
        
        //Debug Code
        //cout<<"ReadNxtmsg_msg_timestamppost: "<<msg.timestamp<<std::endl;
        //cout<<"is last price empty: "<<tokens[15].empty()<< endl;
        
        //Set isSnap for the message
        if (tokens[2]=="snap"){
            msg.isSnap = true;
        } else {
            msg.isSnap = false;
        }
        
        TickData tickdata;
        tickdata.ContractName       = tokens[0];
        tickdata.BestBidPrice       = tokens[4].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[4]);
        tickdata.BestBidAmount      = tokens[5].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[5]);
        tickdata.BestBidIV          = tokens[6].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[6])/100.0;
        tickdata.BestAskPrice       = tokens[7].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[7]);
        tickdata.BestAskAmount      = tokens[8].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[8]);
        tickdata.BestAskIV          = tokens[9].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[9])/100.0;
        tickdata.MarkPrice          = tokens[10].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[10]);
        tickdata.MarkIV             = tokens[11].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[11])/100.0;
        tickdata.UnderlyingIndex    = tokens[12];
        tickdata.UnderlyingPrice    = tokens[13].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[13]);
        tickdata.LastPrice          = tokens[15].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[15]);
        tickdata.OpenInterest       = tokens[16].empty() ? numeric_limits<double>::quiet_NaN() : stod(tokens[16]);
        tickdata.LastUpdateTimeStamp= TimeToUnixMS(tokens[1]);
        
        //calculate midVol using  BestbidIV and BestAskIV
        if (tickdata.BestBidAmount == 0.0 && tickdata.BestAskAmount != 0.0){
            tickdata.midVol = tickdata.BestAskIV;
        } else if (tickdata.BestBidAmount != 0.0 && tickdata.BestAskAmount == 0.0){
            tickdata.midVol = tickdata.BestBidIV;
        } else {
            tickdata.midVol = (tickdata.BestAskIV + tickdata.BestBidIV)/2;
        }
        
        //Set Strike
        size_t delimiterPos1 = tickdata.ContractName.find("-", 0);
        size_t delimiterPos2 = tickdata.ContractName.find("-", delimiterPos1+1);
        size_t delimiterPos3 = tickdata.ContractName.find("-", delimiterPos2+1);
        size_t strike_length =delimiterPos3 - delimiterPos2- 1;
        string strike_str = tickdata.ContractName.substr(delimiterPos2+1,strike_length);
        
        tickdata.strike = stod(strike_str);
        
        //Push the tickdata into the msg
        msg.Updates.push_back(tickdata);
        
        //Debug Code
        //cout<<"Contract Name: "<<tickdata.ContractName<<" BestBidAmount: "<<tickdata.BestBidAmount<<" BestAskAmount: "<<tickdata.BestAskAmount<<" TS: "<<tickdata.LastUpdateTimeStamp<<" tickdata.midVol : "<<tickdata.midVol << ":A:"<<tickdata.BestAskIV<<":B:"<<tickdata.BestBidIV<<" Strike: "<<tickdata.strike<<endl;
        //cout<<endl;
        
        //Just for testing
        if (tickdata.LastUpdateTimeStamp >= 1652572861193){
            return false;
        }
    }
    
    msg.isSet=true;
    
    return true;
}


CsvFeeder::CsvFeeder(const std::string ticker_filename,
                     FeedListener feed_listener,
                     std::chrono::minutes interval,
                     TimerListener timer_listener)
        : ticker_file_(ticker_filename),
          feed_listener_(feed_listener),
          interval_(interval),
timer_listener_(timer_listener) {
    // initialize member variables with input information, prepare for Step() processing
    string line;
    //ifstream infilestream;
    //std::ifstream infilestream;
    //ticker_file_.open(ticker_filename);
    getline(ticker_file_,line);
    std::cout<<"feeder: "<<ticker_file_.is_open()<<std::endl;
    ReadNextMsg(ticker_file_, msg_);
    //cout<<"csvfeeder:error test"<<endl;
    if (msg_.isSet) {
        // initialize interval timer now_ms_
        now_ms_ = msg_.timestamp;
    } else {
        throw std::invalid_argument("empty message at initialization");
    }
}


bool CsvFeeder::Step() {
    if (msg_.isSet) {
        // call feed_listener with the loaded Msg
        feed_listener_(msg_);
        
        //Debug Code
        //std::cout<<"step_msg_timestamp: "<<msg_.timestamp<<std::endl; //Gourav
        //std::cout<<"step_now_ms: "<<now_ms_<<std::endl; //Gourav
        //std::cout<<"step ticker file"<<ticker_file_<<std::endl; //Gourav
        
        // if current message's timestamp is crossing the given interval, call time_listener, change now_ms_ to the next interval cutoff
        if (now_ms_ < msg_.timestamp) {
            timer_listener_(now_ms_);
            now_ms_ += interval_.count();
        }
        // load tick data into Msg
        // if there is no more message from the csv file, return false, otherwise true
        msg_.Updates.clear(); //clear the message
        msg_.isSet=false; //Set isSet to False
        
        return ReadNextMsg(ticker_file_, msg_);
    }
    return false;
}

CsvFeeder::~CsvFeeder() {
    // release resource allocated in constructor, if any
}
