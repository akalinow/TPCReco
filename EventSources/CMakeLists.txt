set(MODULE_NAME "EventSources")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})
reco_add_executable(grawToEventTPC bin/grawToEventTPC.cpp)

# Disable/enable searching next file with N+1 index (default=DISABLED)
option(EVENTSOURCEGRAW_NEXT_FILE_DISABLE
       "Define EVENTSOURCEGRAW_NEXT_FILE_DISABLE variable" ON)
if(EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
  message(STATUS "EventSourceGRAW/EventSourceMultiGRAW automatic next file index disabled")
  target_compile_definitions(${MODULE_NAME}
                             PRIVATE EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
else()
  message(STATUS "EventSourceGRAW/EventSourceMultiGRAW automatic next file index enabled")
endif()

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} 
            DataFormats Utilities EventGenerator UtilsMC
            $<$<BOOL:${GET_FOUND}>:GrawToROOT>)
            
target_link_libraries(grawToEventTPC PRIVATE Boost::program_options ${MODULE_NAME})

reco_install_targets(${MODULE_NAME} grawToEventTPC)

install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)

reco_add_test_subdirectory(test)
