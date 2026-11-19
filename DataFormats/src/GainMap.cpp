#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "TPCReco/GainMap.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
GainMap::GainMap(const std::string & fileName) : myFileName(fileName) {

  std::ifstream file(fileName);
  if(!file) throw std::runtime_error("GainMap: cannot open file: " + fileName);
  std::string line;
  int lineNumber = 0;
  while(std::getline(file, line)){
    ++lineNumber;
    const auto first = line.find_first_not_of(" \t\r");
    if(first==std::string::npos || line[first]=='#') continue;
    std::istringstream fields(line);
    double xLow, xHigh, gain;
    if(!(fields>>xLow>>xHigh>>gain) || xHigh<=xLow){
      throw std::runtime_error("GainMap: malformed line " + std::to_string(lineNumber) + " in " + fileName);
    }
    myCentres.push_back(0.5*(xLow + xHigh));
    myGains.push_back(gain);
  }
  if(myGains.empty()) throw std::runtime_error("GainMap: no bins in file: " + fileName);
  validate();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
GainMap::GainMap(const std::vector<double> & xCentres, const std::vector<double> & gains)
  : myCentres(xCentres), myGains(gains) {

  if(myCentres.size()!=myGains.size()) throw std::runtime_error("GainMap: centres and gains differ in size");
  validate();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void GainMap::validate() const {

  for(std::size_t i=0;i<myGains.size();++i){
    if(!(myGains[i]>0.0)) throw std::runtime_error("GainMap: non-positive gain in " + myFileName);
    if(i>0 && !(myCentres[i]>myCentres[i-1])) throw std::runtime_error("GainMap: bins not ordered in x in " + myFileName);
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double GainMap::getGain(double x) const {

  if(myGains.empty()) return 1.0;
  if(x<=myCentres.front()) return myGains.front();
  if(x>=myCentres.back()) return myGains.back();
  const auto upper = std::upper_bound(myCentres.begin(), myCentres.end(), x);
  const std::size_t i = upper - myCentres.begin();   // myCentres[i-1] <= x < myCentres[i]
  const double f = (x - myCentres[i-1])/(myCentres[i] - myCentres[i-1]);
  return (1.0 - f)*myGains[i-1] + f*myGains[i];
}
