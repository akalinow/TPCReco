set(MODULE_NAME "GrawToROOT")

if(NOT ${GET_FOUND})
  message(WARNING "No GET software found. " ${MODULE_NAME}
                  " will not be build.")
  return()
endif()

message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} DataFormats
                        GET::cobo-frame-graw2frame GET::MultiFrame Utilities)

reco_install_targets(${MODULE_NAME})
