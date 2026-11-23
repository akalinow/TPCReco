#ifndef DirectoryWatch_H
#define DirectoryWatch_H

#include <functional>
#include <string>

#include <TQObject.h>
#include <RQ_OBJECT.h>

class DirectoryWatch: public TQObject{

  RQ_OBJECT("DirectoryWatch")
  
public:
  
  DirectoryWatch();
  void setUpdateInterval(int aInterval); //[ms]
  virtual ~DirectoryWatch(){};
  void watch(const std::string & dirName);

  /// Optional plain C++ receiver of the same notification as the Message() signal
  /// (comma separated list of modified GRAW files). Called from the watch() thread.
  void setCallback(std::function<void(const std::string &)> aCallback) { myCallback = aCallback; }

private:

  void notify(const std::string & fileList);

  int updateInterval;// [ms]
  std::function<void(const std::string &)> myCallback; //! not streamed

};

#endif
