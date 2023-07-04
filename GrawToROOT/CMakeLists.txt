set(MODULE_NAME "GrawToROOT")

if(NOT ${GET_FOUND})
  message(WARNING "No GET software found. " ${MODULE_NAME}
                  " will not be build.")
  return()
endif()

message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

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

reco_install_targets(${MODULE_NAME})
