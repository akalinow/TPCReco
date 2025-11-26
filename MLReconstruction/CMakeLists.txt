set(MODULE_NAME "MLReconstruction")

if(NOT ${TF_FOUND})
  message(WARNING "No TensorFlow found. " ${MODULE_NAME}
                  " will not be build.")
  return()
endif()

message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

include_directories(${MODULE_NAME} PRIVATE ${TF_INCLUDE_DIR})

target_link_libraries(
      ${MODULE_NAME}
      PUBLIC  Utilities DataFormats EventSources FileOutput  ${ROOT_LIBRARIES} ${TF_LIBRARIES})


reco_add_executable(runTensorflow bin/runTensorflow.cpp)
target_link_libraries(runTensorflow PRIVATE Boost::program_options ${MODULE_NAME})
reco_install_targets(${MODULE_NAME} runTensorflow)

install(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX})





