#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
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

// seconds of a run id YYYYMMDDhhmmss (same scale as GrawName::time)
bool runIdTime(long runId, double & time){

  if(runId<19700101000000L || runId>99991231235959L) return false;
  const long s = runId%100, mi = (runId/100)%100, h = (runId/10000)%100;
  const long d = (runId/1000000)%100, mo = (runId/100000000)%100, y = runId/10000000000L;
  if(mo<1 || mo>12 || d<1 || d>31 || h>23 || mi>59 || s>59) return false;
  time = 86400.0*daysFromCivil(y, mo, d) + 3600.0*h + 60.0*mi + s;
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
std::vector<std::string> findGrawRunFiles(const std::string & dataDir, long runId, int chunk){

  namespace fs = boost::filesystem;
  const double maxTimeDifference = 2.0; // [s]
  double runTime = 0;
  if(chunk<0 || !runIdTime(runId, runTime)) return {};

  // directories to search: the run date ones first (YYYYMMDD...), then all others
  const std::string date = std::to_string(runId/1000000);
  std::vector<fs::path> first, others;
  boost::system::error_code error;
  for(fs::directory_iterator it(dataDir, error), end; !error && it!=end; it.increment(error)){
    if(!fs::is_directory(it->path(), error)) continue;
    (it->path().filename().string().rfind(date, 0)==0 ? first : others).push_back(it->path());
  }
  std::sort(first.begin(), first.end());
  std::sort(others.begin(), others.end());
  std::vector<fs::path> directories{fs::path(dataDir)};
  directories.insert(directories.end(), first.begin(), first.end());
  directories.insert(directories.end(), others.begin(), others.end());

  for(const auto & directory: directories){
    // closest file per stream (prefix, e.g. CoBo0_AsAd2)
    std::map<std::string, std::pair<double, std::string>> best;
    for(fs::directory_iterator it(directory, error), end; !error && it!=end; it.increment(error)){
      GrawName candidate;
      if(!parseGrawName(it->path().filename().string(), candidate) || candidate.chunk!=chunk) continue;
      const double difference = std::abs(candidate.time - runTime);
      if(difference>maxTimeDifference) continue;
      auto found = best.find(candidate.prefix);
      if(found==best.end() || difference<found->second.first){
        best[candidate.prefix] = {difference, it->path().string()};
      }
    }
    if(best.empty()) continue;
    std::vector<std::string> files;
    for(const auto & item: best) files.push_back(item.second.second);
    return files;
  }
  return {};
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
