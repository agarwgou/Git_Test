#include "CubicSmile.h"
#include "CsvFeeder.h"
#include "BSAnalytics.h"
#include <iostream>
#include <cmath>
#include "Solver/LBFGS.h"
#include <Eigen/Core>
//#include "Solver/LBFGSB.h"


using namespace LBFGSpp;
using Eigen::VectorXd;
using Eigen::MatrixXd;
typedef double Scalar;
typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> Vector;

//Define Vol Interpolation function to calculate initial parameters of the Solver
//This can even be used as an approximate model on its own
double calculateInterpolatedVol(const std::vector<TickData> & volTickerSnap, double strike){
    double atm_vol_left, atm_vol_right , strike_left, strike_right, interpolated_vol;
    double distance_left=999999999;
    double distance_right=999999999;
    
    for (const auto& ticker : volTickerSnap){
        if ((strike - ticker.strike) >= 0 &&  abs(strike - ticker.strike) < distance_left){
            atm_vol_left = ticker.midVol;
            strike_left=ticker.strike;
            distance_left = abs(strike - ticker.strike);
        }
    }

    for (const auto& ticker : volTickerSnap){
        if ((strike - ticker.strike) < 0 &&  abs(strike - ticker.strike) < distance_right){
            atm_vol_right = ticker.midVol;
            strike_right=ticker.strike;
            distance_right = abs(strike - ticker.strike);
        }
    }
    
    if (distance_left != 999999999 && distance_right != 999999999){
        interpolated_vol= atm_vol_left + (strike - strike_left)*(atm_vol_right - atm_vol_left)/(strike_right-strike_left);
    } else if (distance_left == 999999999 && distance_right != 999999999){
        interpolated_vol=atm_vol_right;
    } else if (distance_left != 999999999 && distance_right == 999999999){
        interpolated_vol=atm_vol_left;
    } else {
        throw std::logic_error("No Vol Data Available");
    }
    
    return interpolated_vol;
    
    
}


//Function to Fit the Market Vols to a cubic spline model. We have used LBFGS-B solver to the optimize the fitting error which is a weighted average of fitting error at diff strikes.
CubicSmile CubicSmile::FitSmile(const datetime_t& expiryDate, const std::vector<TickData>& volTickerSnap) {
    double fwd, T, atmvol, bf25, rr25, bf10, rr10;
    uint64_t currentMaxTimeStamp=0;
    double atm_vol_left, atm_vol_right , strike_left, strike_right;
    vector<double> actualVols;
    vector<double> strikes;
    vector<double> weights;
    
    // Create an instance of the Cubicsmile class with the optimized parameters
    //CubicSmile optimizedCubicsmile(optimizedParameters[0], optimizedParameters[1], optimizedParameters[2], optimizedParameters[3], optimizedParameters[4], optimizedParameters[5], optimizedParameters[6]);

    // TODO (step 3): fit a CubicSmile that is close to the raw tickers
    // - make sure all tickData are on the same expiry and same underlying
    // - get latest underlying price from all tickers based on LastUpdateTimeStamp
    // - get time to expiry T
    // - fit the 5 parameters of the smile, atmvol, bf25, rr25, bf10, and rr10 using L-BFGS-B solver, to the ticker data
    // ....
    // after the fitting, we can return the resulting smile
    
    cout<<"CHECK POINT: CubicSmile::FitSmile using SIMPLE METHOD"<<endl;
    for (const auto& ticker : volTickerSnap)
    {
        //find the tick with latest time stamp and store forward price
        if (ticker.LastUpdateTimeStamp > currentMaxTimeStamp) {
            currentMaxTimeStamp = ticker.LastUpdateTimeStamp;
            fwd = ticker.UnderlyingPrice;
        }
        
        //prepare actualVols, Strikes, Weights required for Solver later
        actualVols.push_back(ticker.midVol);
        strikes.push_back(ticker.strike);
        
        //Weight calculation
        double distance_to_strike= abs(ticker.UnderlyingPrice - ticker.strike);
        double bid_ask_spread = abs(ticker.BestAskPrice-ticker.BestBidPrice);
        
        //double w = 1;
        //double w = sqrt(ticker.OpenInterest)/max((distance_to_strike*bid_ask_spread),0.1);
        //double w = ticker.OpenInterest/max((distance_to_strike*bid_ask_spread),0.1);
        //double w = (ticker.OpenInterest*ticker.OpenInterest)/max(1.0,sqrt(bid_ask_spread/0.0005));
        //double w = sqrt(ticker.OpenInterest)/max((max(distance_to_strike,2000.0)*bid_ask_spread),0.1);
        double w = ticker.OpenInterest/max((max(distance_to_strike,2000.0)*bid_ask_spread),0.1);

        weights.push_back(w);
        
        
        //Debug code
        std::cout <<"OOOOvolTickerSnap: ContractName= "<< ticker.ContractName<<" BestBidIV= "<<ticker.BestBidIV<<" BestAskIV= "<<ticker.BestAskIV<<" UnderlyingPrice: "<<ticker.UnderlyingPrice<<" Lastupdatedtime: "<<ticker.LastUpdateTimeStamp<<" mid_IV: "<<ticker.midVol<<" Strike: "<<ticker.strike<<"Weight: "<<w<<"::"<<distance_to_strike<<"::"<<bid_ask_spread<<"::"<<ticker.OpenInterest<<std::endl;
    }
    
    
    //Calculate TimetoExpiry
    T = expiryDate - datetime_t(2022,05,15); //
    cout<<"OOOOOOExpiry: "<<expiryDate<<" T: "<<T<<"ConvertCurrentTimeStamp"<<currentMaxTimeStamp<<"::::"<<formatTimestamp(currentMaxTimeStamp)<<endl;
    
    
    
    //Rough ATM_VOL calculation to be sent as Initial value fo atmvol to solver
    double distance=999999999;
    for (const auto& ticker : volTickerSnap){
        if ((fwd - ticker.strike) >= 0 &&  abs(fwd - ticker.strike) < distance){
            atm_vol_left = ticker.midVol;
            strike_left=ticker.strike;
            distance = abs(fwd - ticker.strike);
        }
    }
    
    distance=999999999;
        for (const auto& ticker : volTickerSnap){
            if ((fwd - ticker.strike) < 0 &&  abs(fwd - ticker.strike) < distance){
                atm_vol_right = ticker.midVol;
                strike_right=ticker.strike;
                distance = abs(fwd - ticker.strike);
            }
        }
    
    //Debug code
    cout<<"fwd: "<<fwd<<" T: "<<T<< " expirydate: "<<expiryDate<<" currentMaxTimestamp: "<<currentMaxTimeStamp<<" :: "<< datetime_t(currentMaxTimeStamp/1000)<<"strike_right: "<<strike_right<<"strike_left"<<strike_left<<"vol left: "<<atm_vol_left<<"vol right: "<<atm_vol_right<< endl;
    //fwd=3000;
    //T=2;
    



    atmvol=calculateInterpolatedVol(volTickerSnap,fwd); //atm_vol_left + (fwd - strike_left)*(atm_vol_right - atm_vol_left)/(strike_right-strike_left);//(atm_vol_left + atm_vol_right)/2;
    
    std::vector<double> qds{0.1, 0.25,0.5, 0.75, 0.9};
    std::vector<double> quickstrikes(qds.size());
    std::vector<double> vols(qds.size());

    std::transform(qds.begin(), qds.end(), quickstrikes.begin(), [&] (double qd) { return quickDeltaToStrike(qd, fwd, atmvol, T);});
    
    std::transform(quickstrikes.begin(), quickstrikes.end(), vols.begin(), [&] (double qs) { return calculateInterpolatedVol(volTickerSnap,qs);});
    
    for (auto& x : qds){
        cout<<"qds: "<<x<<" "<<endl;
    }
    
    for (auto& x : quickstrikes){
        cout<<"quickstrikes: "<<x<<" "<<endl;
    }
    
    for (auto& x : vols){
        cout<<"vols: "<<x<<" "<<endl;
    }
    
    bf25=((vols[3]+vols[1]) / 2) - atmvol;
    rr25=vols[1]-vols[3];
    bf10=((vols[4]+vols[0]) / 2) - atmvol;
    rr10=vols[0]-vols[4];

    //Debug code
    cout<<"INTERPOLATED atmvol: "<<atmvol<<endl;

    // Using LBFGS Solver:
    
    cout<<"CHECK POINT: CubicSmile::FitSmile using LBFGS SOLVER"<<endl;

    cout<<"fwd: "<<fwd<<"T: "<<T<<endl;
    
    for (const auto& v : actualVols){
        cout<<v<<" ";
    }
    cout<<endl;
      
    for (const auto& s : strikes){
        cout<<s<<" ";
    }
    cout<<endl;
    
    cout<<"INITIAL PARAMS: "<<atmvol<<" "<<bf25<<" "<<rr25<<" "<<bf10<<" "<<rr10<<endl;
    //Create an instance of FittingError which contains the Objective function
    FittingError fittingError(fwd,T,actualVols, strikes, weights);
    
    //Initialize LBFGSB and set max iteration to 100
    LBFGSParam<Scalar> param;
    param.max_iterations = 100;
    LBFGSSolver<Scalar> solver(param);

    //Set Initial Parameters for the Solver
    Vector initialParameters = Vector::Constant(5, 0);
    initialParameters[0]=atmvol;
    initialParameters[1]=bf25;
    initialParameters[2]=rr25;
    initialParameters[3]=bf10;
    initialParameters[4]=rr10;

    //Set Lower and Upper Bounds
    Vector lb = Vector::Constant(5, -std::numeric_limits<Scalar>::infinity());
    Vector ub = Vector::Constant(5, std::numeric_limits<Scalar>::infinity());
    // The third variable is unbounded
    lb[0] = 0;//-std::numeric_limits<Scalar>::infinity();
    //ub[2] = std::numeric_limits<Scalar>::infinity();
    
    //std::vector<double> optimizedParameters(initialParameters.size());
    Scalar minimumError;

    //Solver
    int iterations = solver.minimize(fittingError, initialParameters, minimumError);
    
    std::cout << "Optimization results:" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Optimized parameters: ";
    for (const auto& param : initialParameters) {
        std::cout << param << " ";
    }
    std::cout << std::endl;
    std::cout << "Minimum FITTING ERROR: " << minimumError << std::endl;
    
    //Assign Optimised Parameters to output variables
    atmvol=initialParameters[0];
    bf25=initialParameters[1];
    rr25=initialParameters[2];
    bf10=initialParameters[3];
    rr10=initialParameters[4];

    cout<<"CUBICSMILE:LBFGS Output: "<<atmvol<<" "<<bf25<<" "<<rr25<<" "<<bf10<<" "<<rr10<<endl;
    cout<<"CubicSmile::FitSmile: Hi! I am inside CubicSmile::FitSmile"<<endl;
    return CubicSmile(fwd, T, atmvol, bf25, rr25, bf10, rr10);
}

CubicSmile::CubicSmile( double underlyingPrice, double T, double atmvol, double bf25, double rr25, double bf10, double rr10) {
    T=T;
    atmvol_=atmvol;
    bf25_=bf25;
    rr25_=rr25;
    bf10_=bf10;
    rr10_=rr10;
    futPrice=underlyingPrice;
    
    cout<<"CubicSmile::CubicSmile: Hi! I am inside CubicSmile::CubicSmile"<<endl;
    // convert delta marks to strike vol marks, setup strikeMarks, then call BUildInterp
    double v_qd90 = atmvol + bf10 - rr10 / 2.0;
    double v_qd75 = atmvol + bf25 - rr25 / 2.0;
    double v_qd25 = atmvol + bf25 + rr25 / 2.0;
    double v_qd10 = atmvol + bf10 + rr10 / 2.0;
    
    cout<<"v_qd90: "<<v_qd90<<" v_qd75: "<<v_qd75<<" atmvol: "<<atmvol<<" v_qd25: "<<v_qd25<<" v_qd10: "<<v_qd10<<endl;
    cout<<"CubicSmile::CubicSmile: Hi! I am updating quick Delta to Strike now"<<endl;
    // we use quick delta: qd = N(log(F/K / (atmvol) / sqrt(T))
    double stdev = atmvol * sqrt(T);
    cout<<"stddev: "<<stdev<<" atmvol: "<<atmvol<<" T"<<T<<" underlyingPrice: "<<underlyingPrice<<" stdev: "<<stdev<<endl;;
    double k_qd90 = quickDeltaToStrike(0.9, underlyingPrice, stdev);
    cout<<"CubicSmile::CubicSmile: 0.9 strike: "<< k_qd90<<endl;
    double k_qd75 = quickDeltaToStrike(0.75, underlyingPrice, stdev);
    cout<<"CubicSmile::CubicSmile: 0.75 strike: "<< k_qd75<<endl;
    double k_qd25 = quickDeltaToStrike(0.25, underlyingPrice, stdev);
    cout<<"CubicSmile::CubicSmile: 0.25 strike: "<< k_qd25<<endl;
    double k_qd10 = quickDeltaToStrike(0.1, underlyingPrice, stdev);
    cout<<"CubicSmile::CubicSmile: 0.10 strike: "<< k_qd10<<endl;

    strikeMarks.push_back(std::pair<double, double>(k_qd90, v_qd90));
    strikeMarks.push_back(std::pair<double, double>(k_qd75, v_qd75));
    strikeMarks.push_back(std::pair<double, double>(underlyingPrice, atmvol));
    strikeMarks.push_back(std::pair<double, double>(k_qd25, v_qd25));
    strikeMarks.push_back(std::pair<double, double>(k_qd10, v_qd10));
    cout<<"CubicSmile::CubicSmile: Hi! I am calling buildInterp now"<<endl;
    BuildInterp();
}

void CubicSmile::BuildInterp()
{
    cout<<"CubicSmile::BuildInterp: Hi! I am inside CubicSmile::BuildInterp"<<endl;
  int n = strikeMarks.size();
  // end y' are zero, flat extrapolation
  double yp1 = 0;
  double ypn = 0;
  y2.resize(n);
  vector<double> u(n-1);

  y2[0] = -0.5;
  u[0]=(3.0/(strikeMarks[1].first-strikeMarks[0].first)) *
    ((strikeMarks[1].second-strikeMarks[0].second) / (strikeMarks[1].first-strikeMarks[0].first) - yp1);

  for(int i = 1; i < n-1; i++) {
    double sig=(strikeMarks[i].first-strikeMarks[i-1].first)/(strikeMarks[i+1].first-strikeMarks[i-1].first);
    double p=sig*y2[i-1]+2.0;
    y2[i]=(sig-1.0)/p;
    u[i]=(strikeMarks[i+1].second-strikeMarks[i].second)/(strikeMarks[i+1].first-strikeMarks[i].first)
      - (strikeMarks[i].second-strikeMarks[i-1].second)/(strikeMarks[i].first-strikeMarks[i-1].first);
    u[i]=(6.0*u[i]/(strikeMarks[i+1].first-strikeMarks[i-1].first)-sig*u[i-1])/p;
  }

  double qn=0.5;
  double un=(3.0/(strikeMarks[n-1].first-strikeMarks[n-2].first)) *
    (ypn-(strikeMarks[n-1].second-strikeMarks[n-2].second)/(strikeMarks[n-1].first-strikeMarks[n-2].first));

  y2[n-1]=(un-qn*u[n-2])/(qn*y2[n-2]+1.0);

//  std::cout << "y2[" << n-1 << "] = " << y2[n-1] << std::endl;
  for (int i=n-2;i>=0;i--) {
    y2[i]=y2[i]*y2[i+1]+u[i];
//    std::cout << "y2[" << i << "] = " << y2[i] << std::endl;
  }
}

double CubicSmile::Vol(double strike)
{
  unsigned i;
  // we use trivial search, but can consider binary search for better performance
  for (i = 0; i < strikeMarks.size(); i++ )
    if (strike < strikeMarks[i].first )
      break; // i stores the index of the right end of the bracket

  // extrapolation
  if (i == 0)
    return strikeMarks[i].second;
  if (i == strikeMarks.size() )
    return strikeMarks[i-1].second;

  // interpolate
  double h = strikeMarks[i].first - strikeMarks[i-1].first;
  double a = (strikeMarks[i].first - strike) / h;
  double b = 1 - a;
  double c = (a*a*a - a) * h * h / 6.0;
  double d = (b*b*b - b) * h * h / 6.0;
  return a*strikeMarks[i-1].second + b*strikeMarks[i].second + c*y2[i-1] + d*y2[i];
}
