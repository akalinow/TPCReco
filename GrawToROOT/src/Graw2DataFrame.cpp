#include "Graw2DataFrame.h"

#include <mfm/BitField.h>
#include <mfm/Field.h>
#include <mfm/Frame.h>
#include <mfm/FrameDictionary.h>
#include <mfm/Exception.h>
#include <utl/Logging.h>
#include "get/GDataFrame.h"
#include "get/GDataChannel.h"
#include "get/GDataSample.h"
#include "get/CoBoEvent.h"
#include "get/Channel.h"

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

  size_t frameCount = (frameOffset>0 ? frameOffset : 0 );
  bool accepted=false;
  if(!loadFile(filePath)) return accepted;
  frameCount -= lastFrameRead;
  std::cout<<" frameCount: "<<frameCount<<std::endl;
    		  
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
      // Decode frame
      get::CoBoEvent event;
      event.fromFrame(*frame.get());
      // Reset ROOT frame
      dataFrame.Clear();
      // Get meta-data
      dataFrame.fHeader.fRevision = frame->header().revision();
      dataFrame.fHeader.fDataSource = frame->header().dataSource();
      dataFrame.fHeader.fEventTime = event.eventTime();
      dataFrame.fHeader.fEventIdx = event.eventIdx();
      dataFrame.fHeader.fCoboIdx = event.coboIdx();
      dataFrame.fHeader.fAsadIdx = event.asadIdx();
      dataFrame.fHeader.fReadOffset = event.readOffset();
      dataFrame.fHeader.fStatus = event.status();

      if(!readFullEvent) return accepted;
		    
      for (size_t agetId=0; agetId < 4u; ++agetId)
	{
	  const mfm::DynamicBitset & pattern = event.hitPattern(agetId);
	  for (size_t chanId=0; chanId < GET::GFrameHeader::MAX_CHANNELS; ++chanId)
	    {
	      dataFrame.fHeader.fHitPatterns[agetId].SetBit(chanId, pattern[chanId]);
	    }
	  dataFrame.fHeader.fMult[agetId] = event.multiplicity(agetId);
	}
      dataFrame.fHeader.fWindowOut = event.windowOut();
      for (size_t agetId=0; agetId < 4u; ++agetId)
	{
	  dataFrame.fHeader.fLastCellIdx[agetId] = event.lastCell(agetId);
	}
		  
      // Loop over items
      const uint32_t maxAget = 4u;
      const size_t maxChanPerAget = 68u;
      for (size_t chanIdx=0; chanIdx < maxChanPerAget; ++chanIdx)
	{
	  for (size_t agetIdx=0; agetIdx < maxAget; ++agetIdx)
	    {
	      try
		{
		  // The const version of CoBoEvent::channel throws an exception for non-existing channels
		  // while the non-const version creates the channel
		  const get::CoBoEvent & constEvent = event;
		  const get::Channel & channel = constEvent.channel(chanIdx, agetIdx);
			      
		  GET::GDataChannel* chan = dataFrame.FindChannel(agetIdx, chanIdx);
			      
		  const std::vector< uint16_t > & bucketIndexes = channel.buckIndexes();
		  const std::vector< uint16_t > & values = channel.sampleValues();
		  const size_t numBuckets = channel.sampleCount();
		  for (size_t buckIdx=0; buckIdx < numBuckets; ++buckIdx)
		    {
		      GET::GDataSample* sample = dataFrame.AddSample();
		      sample->Set(bucketIndexes[buckIdx], values[buckIdx]);
		      chan->AddSample(sample);
		    };
		}
	      catch (const get::CoBoEvent::ChannelNotFound &)
		{
		}
	    }
	}
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
    std::cout<<__FUNCTION__<<" lastFrameRead: "<<lastFrameRead<<std::endl;
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

////////////////////////////////////
////////////////////////////////////



