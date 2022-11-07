#include <algorithm> // for std::find()
#include <iostream>
#include <colorText.h>
#include "GeometryStats.h"
#include "UtilsMath.h" // for Utils::NUMERICAL_TOLERANCE

void GeometryStats::FillSections(int dir, int section, int num){
    auto dirStat=directionsStats.find(dir);
    if(dirStat==directionsStats.end()){
      directionsStats.insert(std::pair<int,DirectionStats>(dir,DirectionStats()));
      dirStat=directionsStats.find(dir);
    }
    auto sectStat=dirStat->second.find(section);
    if(sectStat==dirStat->second.end()){
        dirStat->second.insert(std::pair<int,SectionStats>(section,SectionStats(num)));
    }
    else{
        sectStat->second.n++;
        if(sectStat->second.min>num){
            sectStat->second.min=num;
        }
        if(sectStat->second.max<num){
            sectStat->second.max=num;
        }
    }
    auto dirIndex=directionsSectionIndexList.find(dir);
    if(dirIndex==directionsSectionIndexList.end()){
      directionsSectionIndexList.insert(std::pair<int,SectionIndexList>(dir,SectionIndexList()));
      dirIndex=directionsSectionIndexList.find(dir);      
    }
    auto secIndex=std::find( dirIndex->second.begin(), dirIndex->second.end(), section);
    if(secIndex==dirIndex->second.end()){
      dirIndex->second.push_back(section);
    }
}

//this works only with directions without gaps
void GeometryStats::FillMerged(int dir, int num){
    auto dirStat=mergedStats.find(dir);
    if(dirStat==mergedStats.end()){
        mergedStats.insert(std::pair<int,SectionStats>(dir,SectionStats(num)));
    }
    else{
        if(dirStat->second.min>num){
            dirStat->second.min=num;
        }
        if(dirStat->second.max<num){
            dirStat->second.max=num;
        }
        dirStat->second.n=dirStat->second.max-dirStat->second.min+1; //wrong if directions has gaps
    }
}

void GeometryStats::print(){
    for(auto & dirStat:mergedStats){
        std::cout<<"Direction: "<<dirStat.first<<std::endl;
        std::cout<<" Merged Nstrips = "<<dirStat.second.n<<std::endl;
        std::cout<<" Merged Min = "<<dirStat.second.min<<std::endl;
        std::cout<<" Merged Max = "<<dirStat.second.max<<std::endl;
    }
    for(auto & dirStat:directionsStats){
      std::cout<<"Direction: "<<dirStat.first<<std::endl;
      for(auto & sectStat : dirStat.second){
        std::cout<<" Section: "<<sectStat.first<<" Nstrips = "<<sectStat.second.n<<std::endl;
        std::cout<<" Section: "<<sectStat.first<<" Min = "<<sectStat.second.min<<std::endl;
        std::cout<<" Section: "<<sectStat.first<<" Max = "<<sectStat.second.max<<std::endl;
      }
    }
}

void GeometryStats::FillSectionBoundary(int dir, int section, int num, TVector2 start, TVector2 end){
  auto sectionBoundaryList=directionsStripSectionBoundaryList.find(std::pair<int, int>(dir, num));
  if(sectionBoundaryList==directionsStripSectionBoundaryList.end()) {

    ////// DEBUG
    //    std::cout << __FUNCTION__<<  ": Strip [dir=" << dir << ", sec=" << section << ", num=" << num << "]: "
    //	      << " Adding very first section: "
    //	      << "START=[" << start.X() << ", " << start.Y() << "], "
    //	      << "END=[" << end.X() << ", " << end.Y() << "]" << std::endl;
    ////// DEBUG

    // insert brand new section data assuming only one section per strip
    directionsStripSectionBoundaryList[std::pair<int, int>(dir, num)].push_back(StripSectionBoundaryStats(GeometryStats::outside_section,section,start));
    directionsStripSectionBoundaryList[std::pair<int, int>(dir, num)].push_back(StripSectionBoundaryStats(section,GeometryStats::outside_section,end));
    return;
  }

  ////// DEBUG
  //  std::cout << __FUNCTION__<<  ": Strip [dir=" << dir << ", sec=" << section << ", num=" << num << "]: "
  //	    << " Veryfying new section: "
  //	    << "START=[" << start.X() << ", " << start.Y() << "], "
  //	    << "END=[" << end.X() << ", " << end.Y() << "]" << std::endl;
  ////// DEBUG

  // match new section data with existing boundaries
  auto start_matched=false;
  auto end_matched=false;
  for(auto & it : sectionBoundaryList->second) {

    if((it.pos - start).Mod2()<Utils::NUMERICAL_TOLERANCE) { // START point matches existing boundary
      // sanity check
      if(it.next==section) {
	std::cerr << __FUNCTION__ << KRED << ": WARNING: Duplicate section START for strip_dir=" << dir
		  << ", strip_num=" << ", strip_section=" << section << RST << std::endl;
	continue; // avoid duplicates
      }
      if(it.previous==section) {
	std::cerr << __FUNCTION__ << KRED << ": ERROR: Inconsistent section START for strip_dir=" << dir
		  << ", strip_num=" << ", strip_section=" << section << RST << std::endl;
	exit(-1);
      }
      if(!start_matched) {
	start_matched=true;
	it.next=section; // update adjacent section boundary info
	continue;
      }
    }
    if((it.pos - end).Mod2()<Utils::NUMERICAL_TOLERANCE) { // END point matches existing boundary
      // sanity check
      if(it.previous==section) {
	std::cerr << __FUNCTION__ << KRED << ": WARNING: Duplicate section END for strip_dir=" << dir
		  << ", strip_num=" << ", strip_section=" << section << RST << std::endl;
	continue; // avoid duplicates
      }
      if(it.next==section) {
	std::cerr << __FUNCTION__ << KRED << ": ERROR: Inconsistent section END for strip_dir=" << dir
		  << ", strip_num=" << ", strip_section=" << section << RST << std::endl;
	exit(-1);
      }
      if(!end_matched) {
	end_matched=true;
	it.previous=section; // update adjacent section boundary info
	continue;
      }
    }
  }

  // START point does not match any existing boundary
  if(!start_matched) {

    ////// DEBUG
    //    std::cout << __FUNCTION__<<  ": Strip [dir=" << dir << ", sec=" << section << ", num=" << num << "]: "
    //	      << " Section START does not match existing boundary points" << std::endl;
    ////// DEBUG

    directionsStripSectionBoundaryList[std::pair<int, int>(dir, num)].push_back(StripSectionBoundaryStats(GeometryStats::outside_section,section,start));
  }
  // END point does not match any existing boundary
  if(!end_matched) {

    ////// DEBUG
    //        std::cout << __FUNCTION__<<  ": Strip [dir=" << dir << ", sec=" << section << ", num=" << num << "]: "
    //          << " Section END does not match existing boundary points" << std::endl;
    ////// DEBUG

    directionsStripSectionBoundaryList[std::pair<int, int>(dir, num)].push_back(StripSectionBoundaryStats(section,GeometryStats::outside_section,end));
  }
}

int GeometryStats::GetDirStripNSections(int dir, int num) const{ // number of sections per {strip DIR, strip NUM} pair
  auto sectionBoundaryList=directionsStripSectionBoundaryList.find(std::pair<int, int>(dir, num));
  if(sectionBoundaryList==directionsStripSectionBoundaryList.end()) return 0;
  return std::max( (int)sectionBoundaryList->second.size()-1, 1); // NSections=1 => size=2, NSections=3 => size=2, etc.
}

StripSectionBoundaryList GeometryStats::GetStripSectionBoundaryList(int dir, int num) const{ // active area boundaries + section boundaries
  StripSectionBoundaryList empty; // empty vector
  auto sectionBoundaryList=directionsStripSectionBoundaryList.find(std::pair<int, int>(dir, num));
  if(sectionBoundaryList==directionsStripSectionBoundaryList.end()) return empty;
  return sectionBoundaryList->second;
}

bool GeometryStats::operator==(const GeometryStats& B) const {
  if(this->mergedStats != B.mergedStats) return false;
  for(auto & it :B.directionsStats) {
    auto it2=this->directionsStats.find(it.first);
    if(it2==this->directionsStats.end() || it2->second!=it.second) return false;
  }
  for(auto & it :B.directionsSectionIndexList) {
    auto it2=this->directionsSectionIndexList.find(it.first);
    if(it2==this->directionsSectionIndexList.end() || it2->second!=it.second) return false;
  }
  for(auto & it :B.directionsStripSectionBoundaryList) {
    auto it2=this->directionsStripSectionBoundaryList.find(it.first);
    if(it2==this->directionsStripSectionBoundaryList.end() || it2->second!=it.second) return false;
  }
  return true;
}
