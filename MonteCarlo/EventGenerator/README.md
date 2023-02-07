# Event Generator

## Event generation

### Providers

Generation of random variables from various distributions is done with a help of [Provider](../UtilsMC/include/Provider.h) objects. `Provider` is an abstract base class for following types of providers:

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

All providers are registered in an abstract [ObjectFactory](../UtilsMC/include/ObjectFactory.h). To get a `std::unique_ptr` to, for example, given AngleProvider one has to just run:

```C++
#include "AngleProvider.h"

auto prov=ProviderFactory::Create<AngleProvider>("AngleProviderCosIso");
prov->GetAngle(); //Get angle (with default parameter values)
```

Parameters of various types of providers are listed and described [here](#configuring-providers). For a way to set them see `GeneratorSetup::BuildProvider` template method in [GeneratorSetup.cpp](src/GeneratorSetup.cpp).

### Reactions
[Reactions](include/Reaction.h) are used to produce four-momenta of primary particles generated in an event. So far following reactions are available:
* [ReactionTwoProng](include/ReactionTwoProng.h) - a generic two prong event in which gamma particle hits a target nucleus, which then decays into two products
* [ReactionThreeProngDemocratic](include/ReactionThreeProngDemocratic.h) - a reaction, where 12C nucleus is hit by a gamma particle and decays into three alpha particles **simultaneously** - constant R-matrix
* [ReactionThreeProngIntermediate](include/ReactionThreeProngIntermediate.h) - a reaction, where 12C nucleus is hit by a gamma particle and decays into three alpha particles through intermediate state(s) of a given width(s)

Each reaction implements a virtual method `GeneratePrimaries(double gammaMom, const ROOT::Math::Rotation3D &beamToDetRotation)` in which it generates primary particle based on its type, internal kinematics and energy of incoming gamma particle. The 3D rotation matrix passed as a second argument to `GeneratePrimaries` allows for rotation of the momenta of generated particles from BEAM to DET coordinates.

All the decay kinematics calculations happen in the rest frame of the decaying nucleus, products of the decay are then Lorentz-boosted accordingly to get momenta of final particles in the BEAM frame of reference, and are then rotated to have them in the DET frame of reference.

#### ReactionTwoProng
This reaction utilizes two `AngleProvider` objects to generate angular distribution (azimuthal and polar angle with respect to the beam). Momentum vector lengths are calculated from 2-body decay kinematics, incident gamma energy and target nucleus mass.
#### ReactionThreeProngDemocratic
This reaction takes no parameters, it assumes 12C as a target and calculates kinematics of three alpha products based on incident gamma energy and constant R-matrix.
#### ReactionThreeProngIntermediate
This reaction also assumes 12C as a target and three alpha particle as its products but the decay happens through intermediate state (or states). The intermediate state can have non-zero width - at each invocation of the reaction the invariant mass will be selected from Breit-Wigner distribution of a given width and center value. At the first stage the target nucleus decays into alpha particle and intermediate ion, the alpha particle is one of the final ones. Then, the intermediate ion decays into two alpha particles. It is possible to have multiple intermediate states with different masses, widths and branching ratios. This reaction accepts four `AngleProvider` objects for polar and azimuthal angle distributions of both decays.



#### ReactionLibrary
All reactions used in a given configuration of `EventGenerator` are added to the [ReactionLibrary](include/ReactionLibrary.h), which based on the branching ratios selects a reaction to be used in a given event. All events are equipped with `reaction_type` tag, which allows to tell what was the process which generated the event.


### Putting it all together
Generation of the event starts with selection of the main vertex position. `XYProvider` and `ZProvider` are used to randomly select a vertex position in the BEAM frame, which is then rotated to DET frame and shifted by `BeamPosition` (see [details on beam geometry configuration](#beam-geometry)). Next, `ReactionLibrary` selects a reaction and generates primary particles with it (particle momenta are already rotated to DET frame). The vertex position, together with generated primaries are used to construct `SimEvent` - main product of the event generator.

## Configuration

### Configuring providers

Provider configuration is given by a JSON object of the form:

```json
{
  "distribution": "providerName",
  "parameters": {
    "par1": val1,
    "par2": val2
  }
}
```
where `"providerName` is the name of the specific provider, `"par1/2"`, `val1/2` denote parameter names and values, respectively. Parameters of all types of providers are listed below:

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

Example configuration of `AngleProviderE1E2` would look like:

```json
{
  "distribution": "AngleProviderE1E2",
  "parameters": {
    "sigmaE1": 1,
    "sigmaE2": 0,
    "phaseE1E2": 1.5708,
    "phaseCosSign": 1
  }
}
```
**When configuring providers values of all available parameters must be given!**
### Beam geometry