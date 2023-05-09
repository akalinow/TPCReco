#include "TPCReco/Provider.h"
#include <stdexcept>
#include <algorithm>
#include "iostream"
#include <set>

unsigned int Provider::nInstances = 0;
std::unique_ptr<TRandom> Provider::randGen = std::unique_ptr<TRandom>(gRandom);

void Provider::SetSingleParam(const std::string &pname, const double &pval) {
    ValidateParamName(pname);
    paramVals[pname] = pval;
    ValidateParamValues();
}

double Provider::GetParam(const std::string &pname) {
    ValidateParamName(pname);
    return paramVals[pname];
}

void Provider::ValidateParamName(const std::string &pname) {
    if (paramVals.find(pname) == paramVals.end())
        throw std::runtime_error(GetName() + "::ValidateParamName: Unknown parameter \'" + pname + "\'!");
}


void Provider::SetParams(const paramMapType &pars) {
    std::set<paramMapType::key_type> modifiedPars;
    for (auto &p: pars) {
        ValidateParamName(p.first);
        paramVals[p.first] = p.second;
        modifiedPars.insert(p.first);
    }
    auto allParams = GetParamNames();
    if (modifiedPars != allParams) {
        auto missing = std::set<paramMapType::key_type>();
        std::set_difference(allParams.begin(), allParams.end(), modifiedPars.begin(), modifiedPars.end(),
                            std::inserter(missing, missing.begin()));
        std::string msg = "Not all parameter values provided to " + GetName() + "! Missing ones:";
        for (const auto& p: missing)
            msg += " " + p;
        throw std::runtime_error(msg);
    }

    ValidateParamValues();
}


std::set<Provider::paramMapType::key_type> Provider::GetParamNames() {
    std::set<paramMapType::key_type> v;
    std::transform(paramVals.begin(), paramVals.end(),
                   std::inserter(v, v.begin()),
                   [](auto &p) { return p.first; });
    return v;
}

void Provider::PrintParams() {
    std::cout << "Parameters of " << GetName() << ":\n";
    std::for_each(paramVals.begin(), paramVals.end(),
                  [](auto &p) {
                      std::cout << '\t' << p.first << ": " << p.second << "\n";
                  }
    );
    std::cout << std::flush;
}

void Provider::CheckCondition(bool cond, const std::string &message) {
    if (!cond) {
        auto msg = GetName() + "::ValidateParamValues: ";
        msg += message;
        throw std::invalid_argument(msg);
    }
}
