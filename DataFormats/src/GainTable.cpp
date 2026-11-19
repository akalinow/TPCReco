#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "TPCReco/GainTable.h"

constexpr double GainTable::pressureTolerance;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
GainTable::GainTable(const std::string & fileName) : myFileName(fileName) {

  std::ifstream file(fileName);
  if(!file) throw std::runtime_error("GainTable: cannot open file: " + fileName);
  std::string line;
  int lineNumber = 0;
  while(std::getline(file, line)){
    ++lineNumber;
    line = line.substr(0, line.find('#'));
    if(line.find_first_not_of(" \t\r")==std::string::npos) continue;
    std::istringstream fields(line);
    Entry entry;
    if(!(fields>>entry.runFirst>>entry.runLast>>entry.pressure>>entry.gainCal>>entry.gainRaw) ||
       entry.runLast<entry.runFirst || !(entry.pressure>0) || !(entry.gainCal>0) || !(entry.gainRaw>0)){
      throw std::runtime_error("GainTable: malformed line " + std::to_string(lineNumber) + " in " + fileName);
    }
    myEntries.push_back(entry);
  }
  if(myEntries.empty()) throw std::runtime_error("GainTable: no entries in file: " + fileName);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GainTable::find(long runId, double pressure, Entry & entry) const {

  for(const auto & anEntry: myEntries){
    if(runId>=anEntry.runFirst && runId<=anEntry.runLast &&
       std::abs(pressure - anEntry.pressure)<=pressureTolerance){
      entry = anEntry;
      return true;
    }
  }
  return false;
}
