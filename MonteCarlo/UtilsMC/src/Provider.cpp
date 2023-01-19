#include "Provider.h"
#include <stdexcept>

void Provider::SetSingleParam(const std::string& pname, const double &pval)
{
    ValidateParamName(pname);
    paramVals[pname]=pval;
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
