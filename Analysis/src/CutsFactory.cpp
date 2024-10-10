#include "TPCReco/CutsFactory.h"
namespace tpcreco {
namespace cuts {

CutsFactory::CutsFactory(std::unique_ptr<CutsInterface> imp)
    : imp(std::move(imp)) {
  registerCut("NonEmpty", &CutsFactory::makeNonEmpty);
  registerCut("VertexPosition", &CutsFactory::makeVertexPosition);
  registerCut("VertexZSpan", &CutsFactory::makeVertexZSpan);
  registerCut("GlobalZSpan", &CutsFactory::makeGlobalZSpan);
  registerCut("DistanceToBorder", &CutsFactory::makeDistanceToBorder);
  registerCut("RectangularCut", &CutsFactory::makeRectangularCut);
}

CutType CutsFactory::create(const GeometryTPC *geometry,
                            const boost::property_tree::ptree &ptree) const {
  auto type = ptree.get<std::string>("type");
  auto f = constructors.at(type);
  return (this->*f)(geometry, ptree);
}

CutType CutsFactory::makeNonEmpty(const GeometryTPC *,
                                  const boost::property_tree::ptree &) const {
  return imp->makeNonEmpty();
}

CutType CutsFactory::makeVertexPosition(
    const GeometryTPC *geometry,
    const boost::property_tree::ptree &ptree) const {
  auto beam_offset_mm = ptree.get<double>("beam_offset_mm");
  auto beam_slope_rad = ptree.get<double>("beam_slope_rad");
  auto beam_diameter_mm = ptree.get<double>("beam_diameter_mm");
  return imp->makeVertexPosition(beam_offset_mm, beam_slope_rad,
                                 beam_diameter_mm);
}

CutType CutsFactory::makeDistanceToBorder(
    const GeometryTPC *geometry,
    const boost::property_tree::ptree &ptree) const {
  auto margin_mm = ptree.get<double>("margin_mm");
  return imp->makeDistanceToBorder(geometry, margin_mm);
}

CutType
CutsFactory::makeGlobalZSpan(const GeometryTPC *geometry,
                             const boost::property_tree::ptree &ptree) const {
  auto lower_margin_timecells = ptree.get<double>("lower_margin_timecells");
  auto upper_margin_timecells = ptree.get<double>("upper_margin_timecells");
  return imp->makeGlobalZSpan(geometry, lower_margin_timecells,
                              upper_margin_timecells);
}

CutType
CutsFactory::makeVertexZSpan(const GeometryTPC *geometry,
                             const boost::property_tree::ptree &ptree) const {
  auto beam_diameter_mm = ptree.get<double>("beam_diameter_mm");
  return imp->makeVertexZSpan(geometry, beam_diameter_mm);
}

CutType CutsFactory::makeRectangularCut(
    const GeometryTPC *geometry,
    const boost::property_tree::ptree &ptree) const {
  auto minX_mm = ptree.get<double>("minX_mm");
  auto maxX_mm = ptree.get<double>("maxX_mm");
  auto minY_mm = ptree.get<double>("minY_mm");
  auto maxY_mm = ptree.get<double>("maxY_mm");
  return imp->makeRectangularCut(minX_mm, maxX_mm, minY_mm, maxY_mm);
}

} // namespace cuts
} // namespace tpcreco
