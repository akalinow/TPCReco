#ifndef GRAW2DATAFRAME_H
#define GRAW2DATAFRAME_H

#include <string>
#include <fstream>

#include "get/GDataFrame.h"
#include <mfm/Frame.h>

class Graw2DataFrame{

public:

  Graw2DataFrame();
  
 ~Graw2DataFrame();

  bool initialize(const std::string & formatsPath);

  //size_t getGrawFramesNumber(const std::string & filePath);

  bool getGrawFrame(const std::string & filePath, size_t frameOffset, 
		    GET::GDataFrame &dataFrame, bool readFullEvent=true);

private:

  bool loadFile(const std::string & filePath);

  std::ifstream inputFile;
  std::string inputFilePath{""};

  std::auto_ptr<mfm::Frame> frame;
  long int lastFrameRead{0};
  
  
};
#endif 
