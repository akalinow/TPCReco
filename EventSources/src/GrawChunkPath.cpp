#include <cmath>
#include <limits>
#include <regex>

#include <boost/filesystem.hpp>

#include "TPCReco/GrawChunkPath.h"

namespace {

struct GrawName {
  std::string prefix;  // e.g. CoBo0_AsAd0
  double time{0};      // seconds, only differences are used
  int chunk{-1};
};

// days since 1970-01-01 of a proleptic Gregorian date (H. Hinnant's days_from_civil)
long daysFromCivil(long y, unsigned m, unsigned d){
  y -= m<=2;
  const long era = (y>=0 ? y : y-399)/400;
  const unsigned yoe = static_cast<unsigned>(y - era*400);
  const unsigned doy = (153*(m + (m>2 ? -3 : 9)) + 2)/5 + d - 1;
  const unsigned doe = yoe*365 + yoe/4 - yoe/100 + doy;
  return era*146097 + static_cast<long>(doe) - 719468;
}

bool parseGrawName(const std::string & fileName, GrawName & aName){

  static const std::regex pattern(
    R"(^(.+)_(\d{4})-(\d{2})-(\d{2})T(\d{2})[:-](\d{2})[:-](\d{2})\.(\d+)_(\d{4})\.graw$)");
  std::smatch match;
  if(!std::regex_match(fileName, match, pattern)) return false;
  auto number = [&match](int i){ return std::stol(match[i].str()); };
  aName.prefix = match[1].str();
  aName.time = 86400.0*daysFromCivil(number(2), number(3), number(4))
    + 3600.0*number(5) + 60.0*number(6) + number(7)
    + std::stod("0."+match[8].str());
  aName.chunk = static_cast<int>(number(9));
  return true;
}

} // namespace
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string findGrawChunkPath(const std::string & filePath, int chunkOffset){

  namespace fs = boost::filesystem;
  const double maxTimeDifference = 2.0; // [s]

  GrawName current;
  const fs::path path(filePath);
  if(!parseGrawName(path.filename().string(), current)) return "";
  const int chunk = current.chunk + chunkOffset;
  if(chunk<0) return "";

  fs::path directory = path.parent_path();
  if(directory.empty()) directory = ".";
  boost::system::error_code error;
  fs::directory_iterator it(directory, error), end;
  if(error) return "";

  std::string bestPath;
  double bestDifference = std::numeric_limits<double>::max();
  for(; it!=end; it.increment(error)){
    if(error) break;
    GrawName candidate;
    if(!parseGrawName(it->path().filename().string(), candidate)) continue;
    if(candidate.chunk!=chunk || candidate.prefix!=current.prefix) continue;
    const double difference = std::abs(candidate.time - current.time);
    if(difference<=maxTimeDifference && difference<bestDifference){
      bestDifference = difference;
      bestPath = (directory/it->path().filename()).string();
    }
  }
  return bestPath;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
