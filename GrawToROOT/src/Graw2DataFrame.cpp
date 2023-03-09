#include "TPCReco/Graw2DataFrame.h"

#include <mfm/BitField.h>
#include <mfm/Field.h>
#include <mfm/Frame.h>
#include <mfm/FrameDictionary.h>
#include <mfm/Exception.h>
#include <utl/Logging.h>
#include <get/GDataFrame.h>
#include <get/graw2dataframe.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

using mfm::Frame;
using std::strerror;

////////////////////////////////////
////////////////////////////////////
Graw2DataFrame::Graw2DataFrame(){
}
////////////////////////////////////
////////////////////////////////////
bool Graw2DataFrame::initialize(const std::string & formatsFilePath){
  // Loading frame formats from XCFG file
  std::string formatPath = formatsFilePath;
  try
    {
      int nFormatsRead = mfm::FrameDictionary::instance().listFormats();
      if(!nFormatsRead){
	LOG_DEBUG() << "Loading formats from file '" << formatPath << "'";
	mfm::FrameDictionary::instance().addFormats(formatPath);
      }
    }
  catch (const CCfg::Exception & e)
    {
      LOG_ERROR() << e.what() << "\nCould not load frame formats!";
      return false;
    }
  return true;
}
////////////////////////////////////
////////////////////////////////////
Graw2DataFrame::~Graw2DataFrame(){

  inputFile.close();
  
}
////////////////////////////////////
////////////////////////////////////
bool Graw2DataFrame::getGrawFrame(const std::string & filePath,
				  size_t frameOffset, GET::GDataFrame & dataFrame,
				  bool readFullEvent){

  if(readFullEvent) return getGrawFrameFull(filePath, frameOffset, dataFrame);
  else return getGrawFrameHeader(filePath, frameOffset, dataFrame);
  
  return false;
}
////////////////////////////////////
////////////////////////////////////
bool Graw2DataFrame::getGrawFrameFull(const std::string & filePath,
				      size_t frameOffset, GET::GDataFrame & dataFrame){

  return GET::getGrawFrame(filePath, frameOffset, dataFrame);

}
////////////////////////////////////
////////////////////////////////////
bool Graw2DataFrame::getGrawFrameHeader(const std::string & filePath,
					size_t frameOffset, GET::GDataFrame & dataFrame){

  size_t frameCount = (frameOffset>0 ? frameOffset : 0 );
  bool accepted=false;

  if(!loadFile(filePath)) return accepted;
  frameCount -= lastFrameRead;

  // Loop over frames in input file
  try {
    do {
      // Move to the last NON-EMPTY frame
      if(frameCount>0) frameCount--;    // frames in seekFrame() are counted starting from 0

      //LOG_DEBUG() << "Moving to event: " << frameCount;
      Frame::seekFrame(inputFile, frameCount);
      lastFrameRead = frameCount+1;
      // Read LAST frame
      try
	{
	  //LOG_DEBUG() << "Reading LAST frame from file...";
	  frame = Frame::read(inputFile);
	}
      catch (const std::ifstream::failure & e)
	{
	  if (inputFile.rdstate() & std::ifstream::eofbit)
	    {
	      LOG_WARN() << "EOF reached.";
	      break;
	    }
	  else
	    {
	      LOG_ERROR() << "Error reading frame: " << e.what();
	      break;
	    }
	}		  
      // Skip frames with anything other than CoBo data
      if (0x1 != frame->header().frameType() and 0x2 != frame->header().frameType())
	{
	  //LOG_DEBUG() << "Skipping empty event " << frameCount << "..."; // starting from 1
	  continue;
	} 
      else 
	{ 
	  accepted=true;
	  break;
	}
    } while(true);

    if(accepted) {
      // Reset ROOT frame
      dataFrame.Clear();
      // Get meta-data
      dataFrame.fHeader.fRevision = frame->header().revision();
      dataFrame.fHeader.fDataSource = frame->header().dataSource();
      dataFrame.fHeader.fEventTime = frame->headerField("eventTime").value<uint64_t>();
      dataFrame.fHeader.fEventIdx = frame->headerField("eventIdx").value<uint32_t>();
      dataFrame.fHeader.fCoboIdx = frame->headerField("coboIdx").value<uint8_t>();
      dataFrame.fHeader.fAsadIdx = frame->headerField("asadIdx").value<uint8_t>();
      dataFrame.fHeader.fReadOffset = frame->headerField("readOffset").value<uint16_t>();
      dataFrame.fHeader.fStatus = frame->headerField("status").value<uint8_t>();
    }
  }
  catch (const std::exception & e)
    {
      LOG_ERROR() << e.what();
    }
  return accepted;
}
////////////////////////////////////
////////////////////////////////////
bool Graw2DataFrame::loadFile(const std::string & filePath){

  if(true || filePath!=inputFilePath){
    if(inputFile.is_open()) inputFile.close();
    lastFrameRead = 0;
    inputFilePath = filePath;
    inputFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
      inputFile.open(filePath.c_str(), std::ios::in | std::ios::binary);
    }
    catch (const std::ifstream::failure & e){
      LOG_ERROR() << "Could not open file '" << filePath << "': " << strerror(errno);
      return false;
    }
  }
  return true;  
}
////////////////////////////////////
////////////////////////////////////




