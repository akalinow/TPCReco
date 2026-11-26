#ifndef TPCSOFT_EPROVIDER_H
#define TPCSOFT_EPROVIDER_H

#include "TPCReco/Provider.h"

class EProvider : public Provider {
public:
    ~EProvider() override = default;
    virtual double GetEnergy() = 0;
};

class EProviderSingle : public EProvider {
public:
    EProviderSingle();
    double GetEnergy() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(EProviderSingle)
};

class EProviderGaus : public EProvider {
public:
    EProviderGaus();
    double GetEnergy() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(EProviderGaus)
};

/// Beam spectrum (Gaussian: meanE, sigmaE) times the Breit-Wigner cross section of a resonance
/// (resonanceE, resonanceWidth; photon energies [MeV]): the energy distribution of the reactions
/// on a narrow resonance. Sampled from the beam Gaussian, accepted with the probability
/// (Gamma/2)^2 / ((E - E_R)^2 + (Gamma/2)^2) (maximum 1 at E_R).
class EProviderGausBW : public EProvider {
public:
    EProviderGausBW();
    double GetEnergy() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(EProviderGausBW)
};

#endif //TPCSOFT_EPROVIDER_H
