#include "GeometryStats.h"

void GeometryStats::FillSections(int dir,int section, int num){
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
        std::cout<<" Merged N = "<<dirStat.second.n<<std::endl;
        std::cout<<" Merged Min = "<<dirStat.second.min<<std::endl;
        std::cout<<" Merged Max = "<<dirStat.second.max<<std::endl;
    }
    for(auto & dirStat:directionsStats){
      std::cout<<"Direction: "<<dirStat.first<<std::endl;
      for(auto & sectStat : dirStat.second){
        std::cout<<" Section: "<<sectStat.first<<" N = "<<sectStat.second.n<<std::endl;
        std::cout<<" Section: "<<sectStat.first<<" Min = "<<sectStat.second.min<<std::endl;
        std::cout<<" Section: "<<sectStat.first<<" Max = "<<sectStat.second.max<<std::endl;
      }
    }
}