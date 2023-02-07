# Event Generator

## Event generation

### Providers

Generation of random variables from various distributions is done with a help
of [Provider](../UtilsMC/include/Provider.h) objects. `Provider` is an abstract base class for following types of
providers:

* [AngleProvider](include/AngleProvider.h) - for angular distributions
    * `AngleProviderSingle` - single pre-defined value
    * `AngleProviderIso` - angles from isotropic distribution
    * `AngleProviderCosIso` - angle from distribution with flat distribution of cosine of the angle (e.g. for theta in
      spherical coordinates)
    * `AngleProviderE1E2` - angle from distributions specific to nuclear decays, as
      presented [here](https://doi.org/10.1103/PhysRevC.73.055801)
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

All providers are registered in an abstract [ObjectFactory](../UtilsMC/include/ObjectFactory.h). To get
a `std::unique_ptr` to, for example, given AngleProvider one has to just run:

```C++
#include "AngleProvider.h"

auto prov=ProviderFactory::Create<AngleProvider>("AngleProviderCosIso");
prov->GetAngle(); //Get angle value (with default parameter values)
```

Parameters of various types of providers are listed and described [here](#configuring-providers). For a way to set them
see `GeneratorSetup::BuildProvider` template method in [GeneratorSetup.cpp](src/GeneratorSetup.cpp).

## Configuration

### Configuring providers

#### AngleProviderSingle

| Parameter name | Description           | Unit | Limits |
|----------------|-----------------------|:----:|:------:|
| *singleAngle*  | returned single angle | rad  |   -    |

#### AngleProviderIso

| Parameter name | Description   | Unit |        Limits        |
|----------------|---------------|:----:|:--------------------:|
| *minAngle*     | minimal angle | rad  | \>=-pi, <=*maxAngle* |
| *maxAngle*     | maximal angle | rad  | <=pi, \>=*minAngle*  |

#### AngleProviderCosIso

| Parameter name | Description                 | Unit |      Limits       |
|----------------|-----------------------------|:----:|:-----------------:|
| *minCos*       | minimal cosine of the angle |  -   | \>=-1, <=*maxCos* |
| *maxCos*       | maximal cosine of the angle |  -   | <=1, \>=*minCos*  |

#### AngleProviderE1E2

| Parameter name | Description                   | Unit |   Limits    |
|----------------|-------------------------------|:----:|:-----------:|
| *sigmaE1*      | sigma of E1 transition        |  -   |    \>=0     |
| *sigmaE2*      | sigma of E2 transition        |  -   |    \>=0     |
| *phaseE1E2*    | interference phase            | rad  |      -      |
| *phaseCosSign* | sign of the interference term |  -   | ==1 or ==-1 |

#### AngleProviderPhi

| Parameter name | Description              | Unit |    Limits    |
|----------------|--------------------------|:----:|:------------:|
| *polDegree*    | beam polarisation degree |  -   |  \>=0, <=1   |
| *polAngle*     | beam polarisation angle  |  -   | \>=0, <=2*pi |

#### EProviderSingle

| Parameter name | Description            | Unit | Limits |
|----------------|------------------------|:----:|:------:|
| *singleE*      | returned single energy | MeV  |  \>=0  |

#### EProviderGaus

| Parameter name | Description             | Unit | Limits |
|----------------|-------------------------|:----:|:------:|
| *meanE*        | mean value of the gauss | MeV  |  \>=0  |
| *sigmaE*       | sigma of the gauss      | MeV  |  \>=0  |

#### XYProviderSingle

| Parameter name | Description                    | Unit | Limits |
|----------------|--------------------------------|:----:|:------:|
| *singleX*      | returned single *x* coordinate |  mm  |   -    |
| *singleY*      | returned single *y* coordinate |  mm  |   -    |

#### XYProviderGaussTail

| Parameter name | Description                  | Unit | Limits |
|----------------|------------------------------|:----:|:------:|
| *meanX*        | mean value in *x* coordinate |  mm  |   -    |
| *meanY*        | mean value in *y* coordinate |  mm  |   -    |
| *flatR*        | radius of flat distribution  |  mm  |  \>=0  |
| *sigma*        | sigma of the gaussian tail   |  mm  |  \>=0  |

#### ZProviderSingle

| Parameter name | Description                    | Unit | Limits |
|----------------|--------------------------------|:----:|:------:|
| *singleZ*      | returned single *z* coordinate |  mm  |   -    |

#### ZProviderUniform

| Parameter name | Description | Unit |  Limits   |
|----------------|-------------|:----:|:---------:|
| *minZ*         | minimal *z* |  mm  | <=*maxZ*  |
| *maxZ*         | maximal *z* |  mm  | \>=*minZ* |