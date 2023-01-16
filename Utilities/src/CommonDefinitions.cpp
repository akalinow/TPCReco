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

//Keep type definition and dictionary in unnamed namespace not to expose them
namespace{
    typedef boost::bimap<pid_type, std::string> PidDictionary;

    const PidDictionary gPids =
            boost::assign::list_of<PidDictionary::relation>
            (UNKNOWN,        "UNKNOWN")
            (ALPHA,          "ALPHA")
            (CARBON_12,      "CARBON_12")
            (CARBON_14,      "CARBON_14")
            (C12_ALPHA,      "C12_ALPHA")
            (PROTON,         "PROTON")
            (CARBON_13,      "CARBON_14")
            (NITROGEN_15,    "NITROGEN_15")
            (OXYGEN_16,      "OXYGEN_16")
            (OXYGEN_17,      "OXYGEN_17")
            (OXYGEN_18,      "OXYGEN_18")
            (THREE_ALPHA,    "THREE_ALPHA")
            ;
}

pid_type GetPidType(std::string &pidName)
{
    auto  it = gPids.right.find(pidName);
    return it == gPids.right.end() ? pid_type::UNKNOWN : it->second;
}

std::string GetPidName(pid_type type)
{
    auto  it = gPids.left.find(type);
    return it == gPids.left.end() ? "UNKNOWN" : it->second;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
