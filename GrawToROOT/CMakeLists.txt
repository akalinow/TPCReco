set(MODULE_NAME "GrawToROOT")

if(NOT ${GET_FOUND})
  message(WARNING "No GET software found. " ${MODULE_NAME}
                  " will not be build.")
  return()
endif()

message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})
reco_add_executable(grawToEventTPC bin/grawToEventTPC.cpp)
reco_add_executable(grawToEventRaw bin/grawToEventRaw.cpp)
reco_add_executable(rawPedestalAnalysis bin/rawPedestalAnalysis.cpp)

# Enable/disable searching next file with N+1 index (default=ON)
option(EVENTSOURCEGRAW_NEXT_FILE_DISABLE
       "Define EVENTSOURCEGRAW_NEXT_FILE_DISABLE variable" ON)
if(EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
  message(STATUS "EventSourceGRAW automatic next file index disabled")
  target_compile_definitions(${MODULE_NAME}
                             PRIVATE EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
else()
  message(STATUS "EventSourceGRAW automatic next file index enabled")
endif()

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} DataFormats
                        GET::cobo-frame-graw2frame GET::MultiFrame Utilities)
target_link_libraries(grawToEventTPC PRIVATE Boost::program_options
                                             ${MODULE_NAME})
target_link_libraries(grawToEventRaw PRIVATE Boost::program_options
                                             ${MODULE_NAME})
target_link_libraries(rawPedestalAnalysis PRIVATE Boost::program_options
                                                  ${MODULE_NAME})

reco_install_targets(${MODULE_NAME} grawToEventTPC grawToEventRaw
                     rawPedestalAnalysis)

install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)
