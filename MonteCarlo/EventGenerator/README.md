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
Generation of the event starts with selection of the main vertex position. `XYProvider` and `ZProvider` are used to randomly select a vertex position in the BEAM frame, which is then rotated to DET frame and shifted by `BeamPosition` (see [details on beam geometry configuration](#beam-geometry)). Next, `ReactionLibrary` selects a reaction and generates primary particles coming from interaction of target ion with gamma photon with energy selected by `EProvider`(particle momenta are already rotated to DET frame). The vertex position, together with generated primaries are used to construct `SimEvent` - main product of the event generator.

## Configuration

Configuration of the EventGenerator is done with `boost::property_tree::ptree` objects, which are used to parse JSON configuration files. The configuration has the following block structure:

```json
{
  "Beam": {
    "BeamGeometry": {},
    "GammaEnergy": {}
  },
  "Vertex": {
    "VertexTransverse": {},
    "VertexLongitudinal": {}
  },
  "Reactions": [
    {},
    {}
  ]
}
```
Each `{}` in the above JSON represents another JSON object, configuring specific part of the simulation. Those fields are described below:

### Beam geometry
Beam geometry defines relation between BEAM and DET coordinates, using two sets of Euler angles:
* `EulerAnglesNominal` - nominal transformation from DET to BEAM
* `EulerAnglesActual` - fine-tuning of the beam angle to have possibility to introduce beam tilt
  Those angles are used to construct two 3D rotation matrices: *Rnominal* and *Ractual*. Each vector *v* defined in BEAM coordinates (as in the case of event generator) is rotated to DET by:

*v_DET*=(*Ractual* * *Rnominal*)^(-1)**v*

We inverse the transformation, as euler angles tell us how to get form DET to BEAM, and we want to go in the opposite direction. Convention adapted from [CoordinateConverter](../../Utilities/include/CoordinateConverter.h). Additionally, for position reference the `BeamPosition` is provided.

Geometry is stored in JSON configuration in the following form:

```json
{
  "EulerAnglesNominal": {
    "phi": -1.5708,
    "theta": 1.5708,
    "psi": 0.0
  },
  "EulerAnglesActual": {
    "phi": 0.0,
    "theta": 0.0,
    "psi": 0.0
  },
  "BeamPosition": {
    "x": 0,
    "y": 0,
    "z": 0
  }
}
```
Here we have no beam tilt, and the beam passes through `(0,0,0)`.

### GammaEnergy

Beam energy is configured via providing configuration of the selected `EProvider`.

### VertexTransverse

Vertex distribution in direction transverse to the beam axis is configured via providing configuration of the selected `XYProvider`.

### VertexLongitudinal

Vertex distribution along the beam axis is configured via providing configuration of the selected `ZProvider`.

### Reactions

A vector of reactions is provided for the generator to select from. Each reaction has specific configuration. All reactions have the following two fields in common:
* `"branchingRatio"` - `float`, branching ratio used to select reaction from a library if more than one reaction is defined. Reaction selector handles properly cases where sum of BR's is more than one.
* `"tag"` - `string`, name of the `reaction_type` (defined in [CommonDefinitions](../../DataFormats/include/CommonDefinitions.h)).

#### ReactionTwoProng

Configuration template:

```json
{
  "type": "TwoProng",
  "branchingRatio": {},
  "tag": {},
  "target": {},
  "FirstProduct": {},
  "SecondProduct": {},
  "Theta": {},
  "Phi": {}
}
```

where:
* `"target"` - `string`, name of the target particle (from `pid_type` in [CommonDefinitions](../../DataFormats/include/CommonDefinitions.h))
* `"FirstProduct"` - `string`, name of the first product particle (from `pid_type` in [CommonDefinitions](../../DataFormats/include/CommonDefinitions.h))
* `"SecondProduct"` - `string`, name of the second product particle (from `pid_type` in [CommonDefinitions](../../DataFormats/include/CommonDefinitions.h))
* `"Theta"` - `JSON`, configuration of theta `AngleProvider`
* `"Phi"` - `JSON`, configuration of phi `AngleProvider`

#### ReactionThreeProngDemocratic

Configuration template:

```json
 {
  "type": "ThreeProngDemocratic",
  "branchingRatio": {},
  "tag": {}
}
```

No additional parameters so far.

#### ReactionThreeProngIntermediate

Configuration template:

```json
{
  "type": "ThreeProngIntermediate",
  "branchingRatio": {},
  "tag": {},
  "Theta1": {},
  "Phi1": {},
  "Theta2": {},
  "Phi2": {},
  "IntermediateStates": [
    {},
    {}
  ]
}
```
where:
* `"Theta1"` - `JSON`, configuration of theta `AngleProvider` for first decay (alpha+intermediate)
* `"Phi1"` - `JSON`, configuration of phi `AngleProvider` for first decay (alpha+intermediate)
* `"Theta2"` - `JSON`, configuration of theta `AngleProvider` for second decay (alpha+alpha)
* `"Phi2"` - `JSON`, configuration of phi `AngleProvider` for second decay (alpha+alpha)
* `"IntermediateStates"` - vector of `JSON` objects describing intermediate states, 

Each IntermediateStates has a form of:
```json
{
  "mass": {},
  "width": {},
  "branchingRatio": {}
}
```
where:
* `"mass"` - `float`, mass of the state in MeV
* `"width"` - `float`, width of the state in MeV
* `"branchingRatio"` - `float`, BR for selection between different intermediate states

## Example full configuration
Note, that dummy numbers were put in place of relevant physical parameters.

```json
{
  "Beam": {
    "BeamGeometry": {
      "EulerAnglesNominal": {
        "phi": -1.5708,
        "theta": 1.5708,
        "psi": 0.0
      },
      "EulerAnglesActual": {
        "phi": 0.0,
        "theta": 0.0,
        "psi": 0.0
      },
      "BeamPosition": {
        "x": 0,
        "y": 0,
        "z": 0
      }
    },
    "GammaEnergy": {
      "distribution": "EProviderSingle",
      "parameters": {
        "singleE": 11
      }
    }
  },
  "Vertex": {
    "VertexTransverse": {
      "distribution": "XYProviderGaussTail",
      "parameters": {
        "meanX": 0,
        "meanY": 0,
        "flatR": 10,
        "sigma": 1
      }
    },
    "VertexLongitudinal": {
      "distribution": "ZProviderUniform",
      "parameters": {
        "minZ": -100,
        "maxZ": 100
      }
    }
  },
  "Reactions": [
    {
      "type": "TwoProng",
      "branchingRatio": 1,
      "tag": "C12_ALPHA",
      "target": "OXYGEN_16",
      "FirstProduct": "ALPHA",
      "SecondProduct": "CARBON_12",
      "Theta": {
        "distribution": "AngleProviderE1E2",
        "parameters": {
          "sigmaE1": 0,
          "sigmaE2": 1,
          "phaseE1E2": 1.5708,
          "phaseCosSign": 1
        }
      },
      "Phi": {
        "distribution": "AngleProviderPhi",
        "parameters": {
          "polDegree": 0,
          "polAngle": 0
        }
      }
    },
    {
      "type": "ThreeProngDemocratic",
      "branchingRatio": 0,
      "tag": "THREE_ALPHA_DEMOCRATIC"
    },
    {
      "type": "ThreeProngIntermediate",
      "branchingRatio": 1,
      "tag": "THREE_ALPHA_BE",
      "Theta1": {
        "distribution": "AngleProviderCosIso",
        "parameters": {
          "minCos": -1,
          "maxCos": 1
        }
      },
      "Phi1": {
        "distribution": "AngleProviderPhi",
        "parameters": {
          "polDegree": 0,
          "polAngle": 0
        }
      },
      "Theta2": {
        "distribution": "AngleProviderCosIso",
        "parameters": {
          "minCos": -1,
          "maxCos": 1
        }
      },
      "Phi2": {
        "distribution": "AngleProviderPhi",
        "parameters": {
          "polDegree": 0,
          "polAngle": 0
        }
      },
      "IntermediateStates": [
        {
          "mass": 7456.8945,
          "width": 0.01,
          "branchingRatio": 1
        }
      ]
    }
  ]
}
```

## Configuring providers

Provider configuration is given by a JSON object of the form:

```json
{
  "distribution": "providerName",
  "parameters": {
    "par1": {},
    "par2": {}
  }
}
```
where `"providerName` is the name of the specific provider, `"par1/2"`, `{}`s denote parameter names and values, respectively. Parameters of all types of providers are listed below:

### AngleProviderSingle

| Parameter name | Description           | Unit | Limits |
|----------------|-----------------------|:----:|:------:|
| *singleAngle*  | returned single angle | rad  |   -    |

### AngleProviderIso

| Parameter name | Description   | Unit |        Limits        |
|----------------|---------------|:----:|:--------------------:|
| *minAngle*     | minimal angle | rad  | \>=-pi, <=*maxAngle* |
| *maxAngle*     | maximal angle | rad  | <=pi, \>=*minAngle*  |

### AngleProviderCosIso

| Parameter name | Description                 | Unit |      Limits       |
|----------------|-----------------------------|:----:|:-----------------:|
| *minCos*       | minimal cosine of the angle |  -   | \>=-1, <=*maxCos* |
| *maxCos*       | maximal cosine of the angle |  -   | <=1, \>=*minCos*  |

### AngleProviderE1E2

| Parameter name | Description                   | Unit |   Limits    |
|----------------|-------------------------------|:----:|:-----------:|
| *sigmaE1*      | sigma of E1 transition        |  -   |    \>=0     |
| *sigmaE2*      | sigma of E2 transition        |  -   |    \>=0     |
| *phaseE1E2*    | interference phase            | rad  |      -      |
| *phaseCosSign* | sign of the interference term |  -   | ==1 or ==-1 |

### AngleProviderPhi

| Parameter name | Description              | Unit |    Limits    |
|----------------|--------------------------|:----:|:------------:|
| *polDegree*    | beam polarisation degree |  -   |  \>=0, <=1   |
| *polAngle*     | beam polarisation angle  |  -   | \>=0, <=2*pi |

### EProviderSingle

| Parameter name | Description            | Unit | Limits |
|----------------|------------------------|:----:|:------:|
| *singleE*      | returned single energy | MeV  |  \>=0  |

### EProviderGaus

| Parameter name | Description             | Unit | Limits |
|----------------|-------------------------|:----:|:------:|
| *meanE*        | mean value of the gauss | MeV  |  \>=0  |
| *sigmaE*       | sigma of the gauss      | MeV  |  \>=0  |

### XYProviderSingle

| Parameter name | Description                    | Unit | Limits |
|----------------|--------------------------------|:----:|:------:|
| *singleX*      | returned single *x* coordinate |  mm  |   -    |
| *singleY*      | returned single *y* coordinate |  mm  |   -    |

### XYProviderGaussTail

| Parameter name | Description                  | Unit | Limits |
|----------------|------------------------------|:----:|:------:|
| *meanX*        | mean value in *x* coordinate |  mm  |   -    |
| *meanY*        | mean value in *y* coordinate |  mm  |   -    |
| *flatR*        | radius of flat distribution  |  mm  |  \>=0  |
| *sigma*        | sigma of the gaussian tail   |  mm  |  \>=0  |

### ZProviderSingle

| Parameter name | Description                    | Unit | Limits |
|----------------|--------------------------------|:----:|:------:|
| *singleZ*      | returned single *z* coordinate |  mm  |   -    |

### ZProviderUniform

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
