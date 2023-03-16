#ifndef TPCRECO_ANALYSIS_CUTS_FACTORY_H_
#define TPCRECO_ANALYSIS_CUTS_FACTORY_H_
#include "TPCReco/Cuts.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/Track3D.h"
#include <boost/property_tree/ptree.hpp>
#include <functional>
#include <map>
#include <string>

namespace tpcreco {
namespace cuts {

using CutType = std::function<bool(Track3D *)>;

class CutsInterface {
public:
  virtual ~CutsInterface() = default;
  virtual CutType makeNonEmpty() const { return NonEmpty{}; }
  virtual CutType makeVertexPosition(double offset, double slope,
                                     double diameter) const {
    return VertexPosition{offset, slope, diameter};
  }
  virtual CutType makeDistanceToBorder(const GeometryTPC *geometry,
                                       double margin_mm) const {
    return DistanceToBorder{geometry, margin_mm};
  }
  virtual CutType makeGlobalZSpan(const GeometryTPC *geometry,
                                  double lower_margin_timecells,
                                  double upper_margin_timecells) const {
    return GlobalZSpan{geometry, lower_margin_timecells,
                       upper_margin_timecells};
  }
  virtual CutType makeVertexZSpan(const GeometryTPC *geometry,
                                  double beam_diameter_mm) const {
    return VertexZSpan{geometry, beam_diameter_mm};
  }
  virtual CutType makeRectangularCut(double minX_mm, double maxX_mm,
                                     double minY_mm, double maxY_mm) const {
    return RectangularCut{minX_mm, maxX_mm, minY_mm, maxY_mm};
  }
};

class CutsFactory {
public:
  CutsFactory(std::unique_ptr<CutsInterface> imp);
  template <class T> void registerCut(const std::string &name, T &&function) {
    constructors.emplace(name, std::forward<T>(function));
  }
  CutType create(const GeometryTPC *geometry,
                 const boost::property_tree::ptree &ptree) const;

private:
  std::map<std::string,
           CutType (CutsFactory::*)(const GeometryTPC *,
                                    const boost::property_tree::ptree &) const>
      constructors;
  std::unique_ptr<CutsInterface> imp;

  CutType makeNonEmpty(const GeometryTPC *,
                       const boost::property_tree::ptree &) const;

  CutType makeVertexPosition(const GeometryTPC *geometry,
                             const boost::property_tree::ptree &ptree) const;

  CutType makeDistanceToBorder(const GeometryTPC *geometry,
                               const boost::property_tree::ptree &ptree) const;

  CutType makeGlobalZSpan(const GeometryTPC *geometry,
                          const boost::property_tree::ptree &ptree) const;

  CutType makeVertexZSpan(const GeometryTPC *geometry,
                          const boost::property_tree::ptree &ptree) const;

  CutType makeRectangularCut(const GeometryTPC *geometry,
                             const boost::property_tree::ptree &ptree) const;
};
} // namespace cuts
} // namespace tpcreco

#endif // TPCRECO_ANALYSIS_CUTS_FACTORY_H_
