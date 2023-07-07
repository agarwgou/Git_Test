#ifndef QF633_CODE_VOLSURFBUILDER_H
#define QF633_CODE_VOLSURFBUILDER_H

#include <map>
#include "Msg.h"
#include "Date.h"
using namespace std;

template<class Smile>
class VolSurfBuilder {
public:
    void Process(const Msg& msg);  // process message
    void PrintInfo();
    std::map<datetime_t, std::pair<Smile, double> > FitSmiles();
protected:
    // we want to keep the best level information for all instruments
    // here we use a map from contract name to BestLevelInfo, the key is contract name
    std::map<std::string, TickData> currentSurfaceRaw;
};

template <class Smile>
void VolSurfBuilder<Smile>::Process(const Msg& msg) {
    // TODO (Step 2)
    // discard currently maintained market snapshot, and construct a new copy based on the input Msg
    cout<<endl;
    cout<<"Step 2: Writing Message into CurrentSurfaceRaw..."<<endl;
    if (msg.isSnap) {
        currentSurfaceRaw.clear();
        for (const auto& t : msg.Updates)
        {
            //cout<<t.ContractName<< " ";
            string key = t.ContractName;
            currentSurfaceRaw[key]=t;
        }

    } else {  // update the currently maintained market snapshot
        for (const auto& t : msg.Updates)
        {
            //cout<<t.ContractName<< " ";
            string key = t.ContractName;
            //Change 30Jun9am
            auto it = currentSurfaceRaw.find(key);
            
            if (it != currentSurfaceRaw.end() && t.LastUpdateTimeStamp <= it->second.LastUpdateTimeStamp){
                cout<<"Ignoring tickdata for "<<t.ContractName<<" as more recent information already available"<<endl;
            }
                        
            if (it != currentSurfaceRaw.end() && t.LastUpdateTimeStamp > it->second.LastUpdateTimeStamp) {
                // Key exists, and timestamp of new msg > old timestamp (Jira QF633-26) update the value
                it->second = t;
            }
        }
        
    }

    cout<<"Message succesfully written into CurrentSurfaceRaw"<<endl;
    cout<<endl;

}

template <class Smile>
void VolSurfBuilder<Smile>::PrintInfo() {
    // TODO (Step 2): you may print out information about VolSurfBuilder's currentSnapshot to test

    for (const auto& entry : currentSurfaceRaw)
    {
        std::cout <<"Contract Name: "<<entry.first << " ,mid IV : " << entry.second.midVol << std::endl;
    }
}

template <class Smile>
std::map<datetime_t, std::pair<Smile, double> > VolSurfBuilder<Smile>::FitSmiles() {
    std::map<datetime_t, std::vector<TickData> > tickersByExpiry{};
    // TODO (Step 3): group the tickers in the current market snapshot by expiry date, and construct tickersByExpiry
    // ...
    //cout<<"Printing CurrentSurfaceRaw..."<<endl;
    //PrintInfo();
    cout<<"Cleaning Data and Inserting it by Expiry from CurrentSurfaceRaw into ticketbyExpiry..."<<endl;
    for (const auto& entry : currentSurfaceRaw)
    {
        
        //Extract and Convert Expiry Date into datetime_t format
        size_t delimiterPos1 = entry.first.find("-", 0);
        size_t delimiterPos2 = entry.first.find("-", delimiterPos1+1);
        size_t delimiterPos3 = entry.first.find("-", delimiterPos2+1);
        
        //Extract Expiry Date
        size_t delimiterdistance =delimiterPos2 - delimiterPos1 - 1;
        string dd,mm,yy;
        
        //Expiry Date in Ticker is assumed to be in DDMMMYY. It can be 7 characters for 2 digit dates and 6 characters for 1 digit date
        if (delimiterdistance == 7){
            dd = entry.first.substr(4, 2);
            mm = entry.first.substr(6, 3);
            yy = entry.first.substr(9, 2);
        } else {
            dd = entry.first.substr(4, 1);
            mm = entry.first.substr(5, 3);
            yy = entry.first.substr(8, 2);
        }
        
        //cout<<"delimiterPos1: "<<delimiterPos1<<" delimiterPos2: "<<delimiterPos2<<" mm: "<<mm <<endl;
        
        std::unordered_map<std::string, int> monthMap = {
            {"JAN", 1},
            {"FEB", 2},
            {"MAR", 3},
            {"APR", 4},
            {"MAY", 5},
            {"JUN", 6},
            {"JUL", 7},
            {"AUG", 8},
            {"SEP", 9},
            {"OCT", 10},
            {"NOV", 11},
            {"DEC", 12}
        };
        
        int yy_int = 2000 + stoi(yy);
        int mm_int = monthMap[mm];
        int dd_int = stoi(dd);
        
        datetime_t datetime_t_(yy_int,mm_int,dd_int,23,59,59);
        
        //Extract Call Put
        string option_type, strike_str;
        option_type = entry.first.substr(delimiterPos3+1);
        
        //Data Clean up
        
        //If Strike < Underlying Price => Ignore C
        //If Strike > Underlying Price => Ignore P
        //If Bid Amount =0 && Ask Amount=0 then ignore
        
        if (((entry.second.strike < entry.second.UnderlyingPrice && option_type == "P") || (entry.second.strike > entry.second.UnderlyingPrice && option_type == "C")) && (entry.second.BestBidAmount != 0.0 || entry.second.BestAskAmount != 0.0))
        {
            tickersByExpiry[datetime_t_].push_back(entry.second);
        }

        //To keep the tickers sorted in increasing order of the strikes
        for (auto& entry : tickersByExpiry){
            sort(entry.second.begin(), entry.second.end(), [] (const TickData & t1, const TickData & t2) {
                    return t1.strike < t2.strike;
                });
        }

    }
    
    //cout<<"VolSurfBuilder<Smile>::FitSmiles Printing tickersbyExpiry"<<endl;
    //for (const auto& entry : tickersByExpiry){
    //    cout<<"Expiry: "<<entry.first<<" No of Tickers: "<<entry.second.size()<<endl;
    //}
    
    
    std::map<datetime_t, std::pair<Smile, double> > res{};
    // then create Smile instance for each expiry by calling FitSmile() of the Smile
    for (auto iter = tickersByExpiry.begin(); iter != tickersByExpiry.end(); iter++) {
        
        cout<<"Running Fit Smile for: "<<iter->first<<endl;
        auto sm = Smile::FitSmile(iter->first, iter->second);  // TODO: you need to implement FitSmile function in CubicSmile
        double fittingError = 0;
        // TODO (Step 3): we need to measure the fitting error here
        vector<double> weights;
        
        for (const auto& ticker : iter->second){
            double distance_to_strike= abs(ticker.UnderlyingPrice - ticker.strike);
            double bid_ask_spread = abs(ticker.BestAskPrice-ticker.BestBidPrice);
            
            //6 weighting functions considered
            //double w = 1;
            //double w = sqrt(ticker.OpenInterest)/max((distance_to_strike*bid_ask_spread),0.1);
            //double w = ticker.OpenInterest/max((distance_to_strike*bid_ask_spread),0.1);
            //double w = (ticker.OpenInterest*ticker.OpenInterest)/max(1.0,sqrt(bid_ask_spread/0.0005));
            //double w = sqrt(ticker.OpenInterest)/max((max(distance_to_strike,2000.0)*bid_ask_spread),0.1);
            double w = ticker.OpenInterest/max((max(distance_to_strike,2000.0)*bid_ask_spread),0.1);

            weights.push_back(w);
        }

        size_t i=0;
        
        for (const auto& ticker : iter->second)
        {
            fittingError += weights[i]*(ticker.midVol - sm.Vol(ticker.strike))*(ticker.midVol - sm.Vol(ticker.strike));
            
            std::cout <<"volTickerSnap: ContractName= "<< ticker.ContractName<<" BestBidIV= "<<ticker.BestBidIV<<" BestAskIV= "<<ticker.BestAskIV<<" UnderlyingPrice: "<<ticker.UnderlyingPrice<<" Lastupdatedtime: "<<ticker.LastUpdateTimeStamp<<" mid_IV: "<<ticker.midVol<<" Strike: "<<ticker.strike<<" fitted vol: "<<sm.Vol(ticker.strike)<<"sm_atmvol:"<<sm.atmvol_<<std::endl;
            
            ++i;
        }

        cout<<"FITTED PARAMETERS: "<< sm.atmvol_ <<" "<<sm.bf25_<<" "<<sm.rr25_<<" "<<sm.bf10_<<" "<<sm.rr10_<<endl;
        cout<<"STRIKE MARKS: ";
        
        for (auto& strkmrk : sm.strikeMarks){
            cout<<strkmrk.first<<"::"<<strkmrk.second<<" ";
        }
        
        double weights_sum = 0;
        for (size_t i = 0; i< weights.size(); ++i){
            weights_sum += weights[i];
        }
        fittingError /= weights_sum;
        
        //fittingError= fittingError/(iter->second).size();
        cout<<"fitting Error: "<<fittingError<<" size: "<<(iter->second).size()<<endl;
        
        res.insert(std::pair<datetime_t, std::pair<Smile, double> >(iter->first,std::pair<Smile, double>(sm, fittingError)));
    }
    return res;
}

#endif //QF633_CODE_VOLSURFBUILDER_H
