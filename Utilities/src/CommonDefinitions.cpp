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
std::vector<definitions::projection_type> getProjectionsList(){
  return {definitions::projection_type::DIR_U, definitions::projection_type::DIR_V, definitions::projection_type::DIR_W};
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& os, const scale_type& type) {
  try{
    os << enumDict::GetProjectionScaleName(type);
  } catch(std::logic_error &e) {
    os.setstate(std::ios_base::failbit);
  };
  return os;
}

std::istream& operator>>(std::istream& is, scale_type& type) {
  try{
    std::string input;
    is >> input;
    type = enumDict::GetProjectionScaleType(input);
  } catch(std::logic_error &e) {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const filter_type& type) {
  try{
    os << enumDict::GetHitFilterName(type);
  } catch(std::logic_error &e) {
    os.setstate(std::ios_base::failbit);
  };
  return os;
}

std::istream& operator>>(std::istream& is, filter_type& type) {
  try{
    std::string input;
    is >> input;
    type = enumDict::GetHitFilterType(input);
  } catch(std::logic_error &e) {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const event_type& type) {
  try{
    os << enumDict::GetEventSourceName(type);
  } catch(std::logic_error &e) {
    os.setstate(std::ios_base::failbit);
  };
  return os;
}

std::istream& operator>>(std::istream& is, event_type& type) {
  try{
    std::string input;
    is >> input;
    type = enumDict::GetEventSourceType(input);
  } catch(std::logic_error &e) {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const pid_type& type) {
  try{
    os << enumDict::GetPidName(type);
  } catch(std::logic_error &e) {
    os.setstate(std::ios_base::failbit);
  };
  return os;
}

std::istream& operator>>(std::istream& is, pid_type& type) {
  try{
    std::string input;
    is >> input;
    type = enumDict::GetPidType(input);
  } catch(std::logic_error &e) {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const gas_mixture_type& type) {
  try{
    os << enumDict::GetGasMixtureName(type);
  } catch(std::logic_error &e) {
    os.setstate(std::ios_base::failbit);
  };
  return os;
}

std::istream& operator>>(std::istream& is, gas_mixture_type& type) {
  try{
    std::string input;
    is >> input;
    type = enumDict::GetGasMixtureType(input);
  } catch(std::logic_error &e) {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

std::ostream& operator<<(std::ostream& os, const reaction_type& type) {
  try{
    os << enumDict::GetReactionName(type);
  } catch(std::logic_error &e) {
    os.setstate(std::ios_base::failbit);
  };
  return os;
}

std::istream& operator>>(std::istream& is, reaction_type& type) {
  try{
    std::string input;
    is >> input;
    type = enumDict::GetReactionType(input);
  } catch(std::logic_error &e) {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

namespace enumDict {
//Keep type definition and dictionary in unnamed namespace not to expose them
    namespace{

        typedef boost::bimap<::scale_type, std::string> ProjectionScaleDictionary;

        typedef boost::bimap<::filter_type, std::string> HitFilterDictionary;

        typedef boost::bimap<::event_type, std::string> EventSourceDictionary;

        typedef boost::bimap<::pid_type, std::string> PidDictionary;

        typedef boost::bimap<::gas_mixture_type, std::string> GasMixtureDictionary;

        typedef boost::bimap<::reaction_type, std::string> ReactionDictionary;

        const PidDictionary gPids =
                boost::assign::list_of<PidDictionary::relation>
                (pid_type::UNKNOWN,                     "UNKNOWN")
                (pid_type::ALPHA,                       "ALPHA")
                (pid_type::CARBON_12,                   "CARBON_12")
                (pid_type::CARBON_14,                   "CARBON_14")
                (pid_type::C12_ALPHA,                   "C12_ALPHA")
                (pid_type::PROTON,                      "PROTON")
                (pid_type::CARBON_13,                   "CARBON_13")
                (pid_type::NITROGEN_15,                 "NITROGEN_15")
                (pid_type::OXYGEN_16,                   "OXYGEN_16")
                (pid_type::OXYGEN_17,                   "OXYGEN_17")
                (pid_type::OXYGEN_18,                   "OXYGEN_18")
                (pid_type::BERYLLIUM_8,                 "BERYLLIUM_8")
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
        const HitFilterDictionary gHitFilters =
                boost::assign::list_of<HitFilterDictionary::relation>
	        (filter_type::none,                      "none")
	        (filter_type::threshold,                 "threshold")
	        (filter_type::island,                    "island")
	        (filter_type::fraction,                  "fraction")
                ;
        const ProjectionScaleDictionary gProjectionScales =
                boost::assign::list_of<ProjectionScaleDictionary::relation>
	        (scale_type::raw,                        "raw")
	        (scale_type::mm,                         "mm")
                ;
        const EventSourceDictionary gEventSources =
                boost::assign::list_of<EventSourceDictionary::relation>
	        (event_type::EventSourceROOT,            "EventSourceROOT")
	        (event_type::EventSourceMC,              "EventSourceMC")
	        (event_type::EventSourceGRAW,            "EventSourceGRAW")
	        (event_type::EventSourceMultiGRAW,       "EventSourceMultiGRAW")
                ;
        const GasMixtureDictionary gGasMixtures =
                boost::assign::list_of<GasMixtureDictionary::relation>
	        (gas_mixture_type::CO2,                  "CO2")
                ;
    }



    pid_type GetPidType(const std::string &name) {
        auto it = gPids.right.find(name);
        return it == gPids.right.end() ? pid_type::UNKNOWN : it->second;
    }

    std::string GetPidName(pid_type type) {
        auto it = gPids.left.find(type);
        return it == gPids.left.end() ? "UNKNOWN" : it->second;
    }

    reaction_type GetReactionType(const std::string &name) {
        auto it = gReactions.right.find(name);
        return it == gReactions.right.end() ? reaction_type::UNKNOWN : it->second;
    }

    std::string GetReactionName(reaction_type type) {
        auto it = gReactions.left.find(type);
        return it == gReactions.left.end() ? "UNKNOWN" : it->second;
    }

    filter_type GetHitFilterType(const std::string &name) {
        auto it = gHitFilters.right.find(name);
        if(it == gHitFilters.right.end()) throw std::logic_error("Missing dictionary entry for filter_type name");
	return it->second;
    }

    std::string GetHitFilterName(filter_type type) {
        auto it = gHitFilters.left.find(type);
        if(it == gHitFilters.left.end()) throw std::logic_error("Missing dictionary entry for filter_type enumerator");
	return it->second;
    }

    scale_type GetProjectionScaleType(const std::string &name) {
        auto it = gProjectionScales.right.find(name);
        if(it == gProjectionScales.right.end()) throw std::logic_error("Missing dictionary entry for scale_type name");
	return it->second;
    }

    std::string GetProjectionScaleName(scale_type type) {
        auto it = gProjectionScales.left.find(type);
        if(it == gProjectionScales.left.end()) throw std::logic_error("Missing dictionary entry for scale_type enumerator");
	return it->second;
    }

    event_type GetEventSourceType(const std::string &name) {
        auto it = gEventSources.right.find(name);
        if(it == gEventSources.right.end()) throw std::logic_error("Missing dictionary entry for event_type name");
	return it->second;
    }

    std::string GetEventSourceName(event_type type) {
        auto it = gEventSources.left.find(type);
        if(it == gEventSources.left.end()) throw std::logic_error("Missing dictionary entry for event_type enumerator");
	return it->second;
    }

    gas_mixture_type GetGasMixtureType(const std::string &name) {
        auto it = gGasMixtures.right.find(name);
        if(it == gGasMixtures.right.end()) throw std::logic_error("Missing dictionary entry for gas_mixture_type name");
	return it->second;
    }

    std::string GetGasMixtureName(gas_mixture_type type) {
        auto it = gGasMixtures.left.find(type);
        if(it == gGasMixtures.left.end()) throw std::logic_error("Missing dictionary entry for gas_mixture_type enumerator");
	return it->second;
    }
} // namespace enumDict
