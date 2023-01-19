#ifndef TPCSOFT_PROVIDER_H
#define TPCSOFT_PROVIDER_H

#include <memory>
#include <map>
#include "TRandom3.h"


class Provider{
public:
    Provider()
        :randGen{std::make_unique<TRandom3>()} {}
    virtual ~Provider()=default;
    void SetSingleParam(const std::string& pname, const double &pval);
    void SetParams(const std::map<std::string,double> &pars);
    double GetParam(const std::string &pname);
    std::vector<std::string> GetParamNames();
    void PrintParams();
protected:
    std::unique_ptr<TRandom> randGen;
    std::map<std::string, double> paramVals;
    virtual void ValidateParamValues()=0;
private:
    void ValidateParamName(const std::string& pname);
};

#endif //TPCSOFT_PROVIDER_H
