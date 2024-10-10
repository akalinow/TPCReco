#ifndef TPCSOFT_PROVIDER_H
#define TPCSOFT_PROVIDER_H

#include <memory>
#include <map>
#include <set>
#include "TRandom3.h"
#include "ObjectFactory.h"
#include "ObjectRegistrator.h"

class Provider;

typedef utl::ObjectFactory<Provider *, std::string> ProviderFactory;


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
  utl::ObjectRegistrator<_moduleName_, ProviderFactory> fAutoModuleReg{};


class Provider {
public:
    typedef std::map<std::string, double> paramMapType;

    Provider() = default;

    virtual ~Provider() = default;

    void SetSingleParam(const std::string &pname, const double &pval);

    void SetParams(const paramMapType &pars);

    double GetParam(const std::string &pname);

    std::set<paramMapType::key_type> GetParamNames();

    void PrintParams();

    virtual std::string GetName() = 0;

protected:
    static std::unique_ptr<TRandom> randGen;
    paramMapType paramVals;

    virtual void ValidateParamValues() = 0;

    static unsigned int nInstances;

    void CheckCondition(bool cond, const std::string &message);

private:
    void ValidateParamName(const std::string &pname);
};

#endif //TPCSOFT_PROVIDER_H
