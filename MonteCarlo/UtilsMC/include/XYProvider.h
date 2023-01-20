#ifndef TPCSOFT_XYPROVIDER_H
#define TPCSOFT_XYPROVIDER_H

#include "Provider.h"
#include <utility>

class TF2;

class XYProvider : public Provider{
public:
    typedef std::pair<double,double> xyVal;
    ~XYProvider() override =default;
    virtual xyVal GetXY()=0;
};

class XYProviderGaussTail : public XYProvider{
public:
    XYProviderGaussTail();
    xyVal GetXY() override;
protected:
    void ValidateParamValues() override;
private:
    double Profile(double *x, double *par);
    std::unique_ptr<TF2> profileTF2;

    REGISTER_PROVIDER(XYProviderGaussTail)
};

#endif //TPCSOFT_XYPROVIDER_H
