#include "CommonDefinitions.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
projection_type get2DProjectionType(int aStrip_dir){
  return get2DProjectionType(static_cast<projection_type>(aStrip_dir));
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
projection_type get2DProjectionType(projection_type aStrip_dir){
  projection_type projType = projection_type::NONE;
  if(aStrip_dir==projection_type::NONE) projType = projection_type::NONE;
  else if(aStrip_dir==projection_type::DIR_U) projType = projection_type::DIR_TIME_U;
  else if(aStrip_dir==projection_type::DIR_V) projType = projection_type::DIR_TIME_V;
  else if(aStrip_dir==projection_type::DIR_W) projType = projection_type::DIR_TIME_W;
  else{
    throw std::logic_error("get2DProjectionType(): 1D projection_type not convertible to 2D projection type");
  }
  return projType;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
projection_type get1DProjectionType(int aStrip_dir){
  return get1DProjectionType(static_cast<projection_type>(aStrip_dir));
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
projection_type get1DProjectionType(projection_type aStrip_dir){
  projection_type projType = projection_type::NONE;

  if(aStrip_dir==projection_type::NONE) projType = projection_type::NONE;
  else if(aStrip_dir==projection_type::DIR_TIME) projType = projection_type::DIR_TIME;
  else if(aStrip_dir==projection_type::DIR_TIME_U) projType = projection_type::DIR_U;
  else if(aStrip_dir==projection_type::DIR_TIME_V) projType = projection_type::DIR_V;
  else if(aStrip_dir==projection_type::DIR_TIME_W) projType = projection_type::DIR_W;
  else{
    throw std::logic_error("get1DProjectionType(): 2D projection_type not convertible to 1D projection type");
  }
  return projType;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
