
## Structure of EventTPC_tst.cpp:

In file there are define void function that takes arguments:

 - std::shared_ptr<EventTPC> EventPtr =  myEventSource->loadDataFile(dataFileName)->getCurrentEvent()
 - std::map<std::string, double> Test_Reference - defined in dataEventTPC.h
 - std::map<std::string, std::string> Test_Reference_Titles - defined in dataEventTPC.h
  
Those functions are:
 - get1DProjection_Titles_Test
 - get2DProjection_Titles_Test
 - get1DProjection_Test
 - get2DProjection_Test
 - GetTotalCharge_Test
 - GetMaxCharge_Test
 - GetMaxChargePos_Test
 - GetSignalRange_Test
 - GetMultiplicity_Test
 
 Each one compares returns of class EventTPC methods with example data, which are sturcture into maps in dataEventTPC.h.
 
