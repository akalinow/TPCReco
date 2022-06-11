#ifndef __GEOMETRYSTATS_H__
#define __GEOMETRYSTATS_H__
#include <map>
#include <vector>
#include "UtilsMath.h"
#include "TVector2.h"

//Helper struct holding strip numbers of min/max strips and number of strips
struct SectionStats{
  SectionStats(int num):min(num),max(num),n(1){}
  int min; // minimal strip index
  int max; // maximal strip index
  int n;   // actual number of strips, NOTE: (max-min+1) is not always the case!

  bool operator==(const SectionStats& B) const {
    return (this->min==B.min) && (this->max==B.max) && (this->n==B.n);
  }
  inline bool operator!=(const SectionStats& B) const { return !(*this==B); }
};

typedef std::map<int,SectionStats> DirectionStats; // key=strip_DIR
typedef std::vector<int> SectionIndexList;

//Helper struct holding cartesian positions of all section start- and end points
struct StripSectionBoundaryStats{
  StripSectionBoundaryStats(int previous_section, int next_section, TVector2 position):pos(position),previous(previous_section),next(next_section){}
  TVector2 pos; // [mm] absolute position of strip section boundary
  int previous; // valid section index, or GeometryStats::outside_section when outside of UVW active area
  int next;     // valid section index, or GeometryStats::outside_section when outside of UVW active area

  bool operator==(const StripSectionBoundaryStats& B) const {
    return (this->previous==B.previous) && (this->next==B.next) && ((this->pos-B.pos).Mod2()<Utils::NUMERICAL_TOLERANCE);
  }
  inline bool operator!=(const StripSectionBoundaryStats& B) const { return !(*this==B); }
};
typedef std::vector<StripSectionBoundaryStats> StripSectionBoundaryList;

//Wrapper around stats maps. 
//Provides interface for strip number for min/max strip and number of strips in direction->section 
//or direction with merged sections
class GeometryStats{
  public:
    GeometryStats()=default;
    inline  void Fill(int dir, int section, int num, TVector2 start, TVector2 end) {
      FillSections(dir, section, num);
      FillMerged(dir, num);
      FillSectionBoundary(dir, section, num, start, end);
    }
    void print();

    inline  SectionIndexList GetDirSectionIndexList(int dir){return directionsSectionIndexList.at(dir);} // added by MC - 4 Aug 2021
    inline  int GetDirNStrips(int dir,int section){return directionsStats.at(dir).at(section).n;}
    inline  int GetDirMinStrip(int dir,int section){return directionsStats.at(dir).at(section).min;}
    inline  int GetDirMaxStrip(int dir,int section){return directionsStats.at(dir).at(section).max;}
    inline  int GetDirNStripsMerged(int dir){return mergedStats.at(dir).n;}
    inline  int GetDirMinStripMerged(int dir){return mergedStats.at(dir).min;}
    inline  int GetDirMaxStripMerged(int dir){return mergedStats.at(dir).max;}
    static const int outside_section{-99}; // index for dummy sections outside of UVW active area used to truncate toy MC generated signals
    inline  int GetDirNSections(int dir){return directionsSectionIndexList.at(dir).size();} // total number of sections per strip DIR
    int GetDirStripNSections(int dir, int num); // number of sections per {strip DIR, strip NUM} pair
    StripSectionBoundaryList GetStripSectionBoundaryList(int dir, int num); // active area boundaries + section boundaries

    bool operator==(const GeometryStats&) const;
    inline bool operator!=(const GeometryStats& B) const { return !(*this==B); }

 private:
    std::map<int,DirectionStats> directionsStats; // key=strip_DIR
    std::map<int,SectionIndexList> directionsSectionIndexList; // key=strip_DIR
    DirectionStats mergedStats;
    void FillSections(int dir, int section,int num);
    void FillMerged(int dir,int num);
    std::map<std::pair<int, int>, StripSectionBoundaryList> directionsStripSectionBoundaryList; // key=[strip DIR, strip NUM]
    void FillSectionBoundary(int dir, int section, int num, TVector2 start, TVector2 end);
};

#endif
