#include "TPCReco/CommonDefinitions.h"

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
