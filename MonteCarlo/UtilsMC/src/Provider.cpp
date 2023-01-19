#include "Provider.h"
#include <stdexcept>
#include <algorithm>
#include "iostream"

void Provider::SetSingleParam(const std::string& pname, const double &pval)
{
    ValidateParamName(pname);
    paramVals[pname]=pval;
    ValidateParamValues();
}

double Provider::GetParam(const std::string &pname)
{
    ValidateParamName(pname);
    return paramVals[pname];
}

void Provider::ValidateParamName(const std::string &pname)
{
    if(paramVals.find(pname)==paramVals.end())
        throw std::runtime_error("Provider::ValidateParamName: Unknown parameter \'"+pname+"\'!");
}

void Provider::SetParams(const std::map<std::string, double> &pars)
{
    for(auto &p: pars)
    {
        ValidateParamName(p.first);
        paramVals[p.first]=p.second;
    }
    ValidateParamValues();
}

std::vector<std::string> Provider::GetParamNames()
{
    std::vector<std::string> v;
    std::transform(paramVals.begin(), paramVals.end(),
                          std::back_inserter(v),
                          [](auto &p){return p.first;});
    return v;
}

void Provider::PrintParams()
{
    std::cout<<"Parameters:\n";
    std::for_each(paramVals.begin(),paramVals.end(),
                  [](auto &p){
                        std::cout<<'\t'<<p.first<<": "<<p.second<<"\n";
                    }
                  );
    std::cout<<std::flush;
}
