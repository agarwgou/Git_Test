#ifndef _CUBICSMILE_H
#define _CUBICSMILE_H

#include <vector>
#include <utility>
#include "Msg.h"
#include "Date.h"
#include "Solver/Eigen/Core"

using namespace std;
typedef double lbfgsfloatval_t;
typedef double Scalar;
typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> Vector;
// CubicSpline interpolated smile, extrapolate flat
class CubicSmile
{
 public:
  static CubicSmile FitSmile(const datetime_t& expiryDate,const std::vector<TickData>&); // FitSmile creates a Smile by fitting the smile params to the input tick data, it assume the tickData are of the same expiry
  // constructor, given the underlying price and marks, convert them to strike to vol pairs (strikeMarks), and construct cubic smile
  CubicSmile( double underlyingPrice, double T, double atmvol, double bf25, double rr25, double bf10, double rr10); // convert parameters to strikeMarks, then call BuildInterp() to create the cubic spline interpolator
  double Vol(double strike); // interpolate

    double atmvol_;
    double bf25_;
    double rr25_;
    double bf10_;
    double rr10_;
    double futPrice;
    
 public: //changed from private for testing
  void BuildInterp();
  // strike to implied vol marks

  vector< pair<double, double> > strikeMarks;
  vector<double> y2; // second derivatives
};

class FittingError {
public:
    FittingError(const double& fwd, const double& tte, const std::vector<double>& actualVols, const std::vector<double>& strikes,const std::vector<double>& weights)
        : actualVols_(actualVols), strikes_(strikes), fwd(fwd), tte(tte), weights_(weights) {}

    double operator()(const Vector& parameters, Vector& grad) {
        
        /*CubicSmile cubicsmile(fwd, tte, parameters[0], parameters[1], parameters[2], parameters[3], parameters[4]);
        
        //fitting error
        double fittingError = 0.0;
        for (size_t i = 0; i < actualVols_.size(); ++i) {
            double interpolatedVol = cubicsmile.Vol(strikes_[i]);
            double error = actualVols_[i] - interpolatedVol;
            fittingError += error * error;
        }
        fittingError /= actualVols_.size();
*/
        
        double fittingError = fittingErrorCalc(parameters);
        cout<<"PARAMATERS: "<<parameters[0]<<" "<<parameters[1]<<" "<<parameters[2]<<" "<<parameters[3]<<" "<<parameters[4]<<" "<<endl;
        cout<<"FITTING ERROR: "<<fittingError<<endl;
        //gradient
        Vector parameters_0,parameters_1,parameters_2,parameters_3,parameters_4;
        
        parameters_0 =parameters;
        parameters_0[0]=parameters[0] + 0.0000001;
        
        parameters_1 =parameters;
        parameters_1[1] =parameters[1] + 0.0000001;
        
        parameters_2 =parameters;
        parameters_2[2] =parameters[2] + 0.0000001;
        
        parameters_3 =parameters;
        parameters_3[3] =parameters[3] + 0.0000001;
        
        parameters_4 =parameters;
        parameters_4[4] =parameters[4] + 0.0000001;

        grad[0]=(fittingErrorCalc(parameters_0) - fittingError)/0.0000001;
        grad[1]=(fittingErrorCalc(parameters_1) - fittingError)/0.0000001;
        grad[2]=(fittingErrorCalc(parameters_2) - fittingError)/0.0000001;
        grad[3]=(fittingErrorCalc(parameters_3) - fittingError)/0.0000001;
        grad[4]=(fittingErrorCalc(parameters_4) - fittingError)/0.0000001;
        
        return fittingError;
    }
    
    double fittingErrorCalc(const Vector& parameters) {
        CubicSmile cubicsmile(fwd, tte, parameters[0], parameters[1], parameters[2], parameters[3], parameters[4]);
        
        //fitting error
        double fittingError = 0.0;
        for (size_t i = 0; i < actualVols_.size(); ++i) {
            double interpolatedVol = cubicsmile.Vol(strikes_[i]);
            double error = actualVols_[i] - interpolatedVol;
            fittingError += weights_[i]*error * error;
        }
        
        double weights_sum = 0;
        for (size_t i = 0; i< weights_.size(); ++i){
            weights_sum += weights_[i];
        }
        cout<<"Fitting Error Calc"<<fittingError<<" "<<weights_sum<<" "<<weights_.size()<<endl;
        fittingError /= weights_sum; //actualVols_.size();
        cout<<fittingError<<endl;
        
        return fittingError;
    }

    /*
    static lbfgsfloatval_t evaluate(
        void* instance,
        const lbfgsfloatval_t* x,
        lbfgsfloatval_t* g,
        const int n,
        const lbfgsfloatval_t step
    ) {
        FittingError* fittingError = reinterpret_cast<FittingError*>(instance);

        Vector parameters(x, x + n);
        Vector m_grad;
        m_grad.resize(7);
        double fittingErrorValue = fittingError->operator()(parameters,m_grad);

        // Calculate the gradient of the fitting error if needed
        // You can update the g array to provide the gradient information

        return fittingErrorValue;
    }
     */

private:
    std::vector<double> actualVols_;
    std::vector<double> strikes_;
    std::vector<double> weights_;
    double tte;
    double fwd;
};

#endif
