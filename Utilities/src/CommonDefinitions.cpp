#include "TPCReco/CommonDefinitions.h"
#include <boost/bimap.hpp>
#include <boost/assign.hpp>

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
definitions::projection_type get2DProjectionType(int aStrip_dir){
  return get2DProjectionType(static_cast<definitions::projection_type>(aStrip_dir));
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
definitions::projection_type get2DProjectionType(definitions::projection_type aStrip_dir){
  definitions::projection_type projType = definitions::projection_type::NONE;
  if(aStrip_dir==definitions::projection_type::NONE) projType = definitions::projection_type::NONE;
  else if(aStrip_dir==definitions::projection_type::DIR_U) projType = definitions::projection_type::DIR_TIME_U;
  else if(aStrip_dir==definitions::projection_type::DIR_V) projType = definitions::projection_type::DIR_TIME_V;
  else if(aStrip_dir==definitions::projection_type::DIR_W) projType = definitions::projection_type::DIR_TIME_W;
  else{
    throw std::logic_error("get2DProjectionType(): 1D definitions::projection_type not convertible to 2D projection type");
  }
  return projType;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
definitions::projection_type get1DProjectionType(int aStrip_dir){
  return get1DProjectionType(static_cast<definitions::projection_type>(aStrip_dir));
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
definitions::projection_type get1DProjectionType(definitions::projection_type aStrip_dir){
  definitions::projection_type projType = definitions::projection_type::NONE;

  if(aStrip_dir==definitions::projection_type::NONE) projType = definitions::projection_type::NONE;
  else if(aStrip_dir==definitions::projection_type::DIR_TIME) projType = definitions::projection_type::DIR_TIME;
  else if(aStrip_dir==definitions::projection_type::DIR_TIME_U) projType = definitions::projection_type::DIR_U;
  else if(aStrip_dir==definitions::projection_type::DIR_TIME_V) projType = definitions::projection_type::DIR_V;
  else if(aStrip_dir==definitions::projection_type::DIR_TIME_W) projType = definitions::projection_type::DIR_W;
  else{
    throw std::logic_error("get1DProjectionType(): 2D definitions::projection_type not convertible to 1D projection type");
  }
  return projType;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

namespace enumDict {
//Keep type definition and dictionary in unnamed namespace not to expose them
    namespace{
        typedef boost::bimap<::pid_type, std::string> PidDictionary;

        typedef boost::bimap<::reaction_type, std::string> ReactionDictionary;

        const PidDictionary gPids =
                boost::assign::list_of<PidDictionary::relation>
                (pid_type::UNKNOWN,                     "UNKNOWN")
                (pid_type::ALPHA,                       "ALPHA")
                (pid_type::CARBON_12,                   "CARBON_12")
                (pid_type::CARBON_14,                   "CARBON_14")
                (pid_type::C12_ALPHA,                   "C12_ALPHA")
                (pid_type::PROTON,                      "PROTON")
                (pid_type::CARBON_13,                   "CARBON_14")
                (pid_type::NITROGEN_15,                 "NITROGEN_15")
                (pid_type::OXYGEN_16,                   "OXYGEN_16")
                (pid_type::OXYGEN_17,                   "OXYGEN_17")
                (pid_type::OXYGEN_18,                   "OXYGEN_18")
                (pid_type::THREE_ALPHA,                 "THREE_ALPHA")
                ;
        const ReactionDictionary gReactions =
                boost::assign::list_of<ReactionDictionary::relation>
                (reaction_type::UNKNOWN,                "UNKNOWN")
                (reaction_type::C12_ALPHA,              "C12_ALPHA")
                (reaction_type::C13_ALPHA,              "C13_ALPHA")
                (reaction_type::C14_ALPHA,              "C14_ALPHA")
                (reaction_type::N15_PROTON,             "N15_PROTON")
                (reaction_type::THREE_ALPHA_DEMOCRATIC, "THREE_ALPHA_DEMOCRATIC")
                (reaction_type::THREE_ALPHA_BE,         "THREE_ALPHA_BE")
                (reaction_type::PARTICLE_GUN,           "PARTICLE_GUN")
                ;
    }



    pid_type GetPidType(const std::string &pidName) {
        auto it = gPids.right.find(pidName);
        return it == gPids.right.end() ? pid_type::UNKNOWN : it->second;
    }

    std::string GetPidName(pid_type type) {
        auto it = gPids.left.find(type);
        return it == gPids.left.end() ? "UNKNOWN" : it->second;
    }

    reaction_type GetReactionType(const std::string &reactionName) {
        auto it = gReactions.right.find(reactionName);
        return it == gReactions.right.end() ? reaction_type::UNKNOWN : it->second;
    }

    std::string GetReactionName(reaction_type type) {
        auto it = gReactions.left.find(type);
        return it == gReactions.left.end() ? "UNKNOWN" : it->second;
    }
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
