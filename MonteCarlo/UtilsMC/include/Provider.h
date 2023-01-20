#ifndef TPCSOFT_PROVIDER_H
#define TPCSOFT_PROVIDER_H

#include <memory>
#include <map>
#include "TRandom3.h"
#include "ObjectFactory.h"
#include "ObjectRegistrator.h"

class Provider;

typedef utl::ObjectFactory<Provider*, std::string> ProviderFactory;


//Produce raw pointer here, we want to down-cast
// it later and with unique_ptr it is tricky
#define REGISTER_PROVIDER(_moduleName_)                                  \
public:                                                                  \
  static std::string GetRegistrationId()                                 \
  { return #_moduleName_; }                                              \
                                                                         \
  static                                                                 \
  Provider*                                                              \
  Create()                                                               \
  {                                                                      \
    return new _moduleName_;                                             \
  }                                                                      \
  std::string GetName() override                                         \
  {                                                                      \
    return #_moduleName_;                                                \
  }                                                                      \
private:                                                                 \
  utl::ObjectRegistrator<_moduleName_, ProviderFactory> fAutoModuleReg;


class Provider{
public:
    Provider()
        :randGen{std::make_unique<TRandom3>(0)} {}
    virtual ~Provider()=default;
    void SetSingleParam(const std::string& pname, const double &pval);
    void SetParams(const std::map<std::string,double> &pars);
    double GetParam(const std::string &pname);
    std::vector<std::string> GetParamNames();
    void PrintParams();
    virtual std::string GetName()=0;
protected:
    std::unique_ptr<TRandom> randGen;
    std::map<std::string, double> paramVals;
    virtual void ValidateParamValues()=0;
private:
    void ValidateParamName(const std::string& pname);
};

#endif //TPCSOFT_PROVIDER_H
