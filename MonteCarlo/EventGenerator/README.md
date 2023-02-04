# Event Generator
## Event generation
### Providers
Generation of random variables from various distributions is done with a help of [Provider](../UtilsMC/include/Provider.h) objects. `Provider` is an abstract base class for following types of providers:
* [AngleProvider](include/AngleProvider.h) - for angular distributions
  * `AngleProviderSingle` - single pre-defined value
  * `AngleProviderIso` - angles from isotropic distribution
  * `AngleProviderCosIso` - angle from distribution with flat distribution of cosine of the angle (e.g. for theta in spherical coordinates)
  * `AngleProviderE1E2` - angle from distributions specific to nuclear decays, as presented [here](https://doi.org/10.1103/PhysRevC.73.055801)
  * `AngleProviderPhi` - phi provider with beam polarisation
* [EProvider](include/EProvider.h) - for gamma energy distribution
  * `EProviderSingle` - single pre-defined energy
  * `EProviderGaus` - energy from a gaussian distribution
* [XYProvider](include/XYProvider.h) - for position distribution in plane perpendicular to the beam axis
  * `XYProviderSingle` - single pre-defined value
  * `XYProviderGaussTail` - flat distribution with gaussian tails
* [ZProvider](include/ZProvider.h) - for position distribution along the beam axis
  * `ZProviderSingle` - single pre-defined value
  * `ZProviderUniform` - uniform distribution

All providers are registered in an abstract [ObjectFactory](../UtilsMC/include/ObjectFactory.h). To get a `std::unique_ptr` to any given provider one has to just run:

```C++
#include "AngleProvider.h"

auto prov=ProviderFactory::Create<AngleProvider>("AngleProviderCosIso");
prov->GetAngle(); //Get angle value (with default parameter values)
```

Parameters of various types of providers are listed and described [here](#configuring-providers) 
## Configuration
### Configuring providers
#### AngleProviderSingle
Parameters:
* *singleAngle* -  returned single angle, any value

#### AngleProviderIso
Parameters:
* *minAngle* - minimal angle
