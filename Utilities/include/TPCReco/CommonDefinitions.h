#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__

#include <map>
#include <iostream>
#include <functional>
#include <memory>
#include <vector>

enum class scale_type{
		 raw,
		 mm
};

enum class filter_type{
		 none,
		 threshold,
		 island,
		 fraction
};

namespace definitions {
enum  projection_type{
  NONE = -1,
  DIR_U=0,          // U-direction channel index
  DIR_V=1,          // V-direction channel index
  DIR_W=2,          // W-direction channel index
  DIR_XY=3,         // 2D projection on XY plane
  DIR_XZ=4,         // 2D projection on XZ plane
  DIR_YZ=5,         // 2D projection on YZ plane
  DIR_3D=6,          // 3D reconstruction
  DIR_TIME_U,
  DIR_TIME_V,
  DIR_TIME_W,
  DIR_TIME,
};

enum class fit_type{
  TANGENT,    // Fit only track segment tangent
  BIAS_Z,       // Fit only track segment bias. Move along time direction. 
  BIAS_XY,       // Fit only track segment bias. Move in the strip plane.
  TANGENT_BIAS, // Fit both track segment bias and tangent
  START_STOP // Fit both track segment start and stop
};

} //namespace definitions

enum class event_type {
	EventSourceROOT,
	EventSourceMC,
	EventSourceGRAW,
	EventSourceMultiGRAW
};



std::ostream& operator<<(std::ostream& os, const event_type& et);
std::istream& operator>>(std::istream& is, event_type& et);

definitions::projection_type get2DProjectionType(int aStrip_dir);
definitions::projection_type get2DProjectionType(definitions::projection_type aStrip_dir);

definitions::projection_type get1DProjectionType(int aStrip_dir);
definitions::projection_type get1DProjectionType(definitions::projection_type aStrip_dir);

std::vector<definitions::projection_type> getProjectionsList(); 

enum pid_type{
  UNKNOWN=0,          //           <-- for backward compatibilty with data analyzed before 20 May 2022
  ALPHA=1,            // Helium-4  <-- for backward compatibilty with data analyzed before 20 May 2022
  CARBON_12=2,        // Carbon-12 <-- for backward compatibilty with data analyzed before 20 May 2022
  CARBON_14=3,        // Carbon-14 <-- for backward compatibilty with data analyzed before 20 May 2022
  C12_ALPHA=4,        //           <-- for backward compatibilty with data analyzed before 20 May 2022
  PROTON,             // Hydrogen
  CARBON_13,          // Carbon-13
  NITROGEN_15,        // Nitrogen-15
  OXYGEN_16,          // Oxygen-16
  OXYGEN_17,          // Oxygen-17
  OXYGEN_18,          // Oxygen-18
  BERYLLIUM_8,        // Beryllium-8
  HELIUM_4=ALPHA,     // alias
  C_12=CARBON_12,     // alias
  C_13=CARBON_13,     // alias
  C_14=CARBON_14,     // alias
  N_15=NITROGEN_15,   // alias
  O_16=OXYGEN_16,     // alias
  O_17=OXYGEN_17,     // alias
  O_18=OXYGEN_18,     // alias
  BE_8=BERYLLIUM_8,   // alias
  THREE_ALPHA,        //
  DOT,                // a dot like track
  PID_MIN=ALPHA,      // alias
  PID_MAX=BERYLLIUM_8 // alias
};

enum gas_mixture_type{
  CO2=1,            // Carbon dioxide
  GAS_MIN=CO2,
  GAS_MAX=CO2
};

enum class reaction_type{
  UNKNOWN=0,
  C12_ALPHA,
  C13_ALPHA,
  C14_ALPHA,
  N15_PROTON,
  THREE_ALPHA_DEMOCRATIC,
  THREE_ALPHA_BE,
  PARTICLE_GUN
};

namespace enumDict {
    //conversion enum <--> string for pid_type
    pid_type GetPidType(const std::string &pidName);
    std::string GetPidName(pid_type type);

    //conversion enum <--> string for reaction_type
    reaction_type GetReactionType(const std::string &reactionName);
    std::string GetReactionName(reaction_type type);
}

////////////////////////////////////////////////////////////////////////////////


#define _endl_ " (" << __FILE__ << "; " << __LINE__ << ")\n"
#define checkpoint std::cout << "checkpoint" << _endl_



#endif
