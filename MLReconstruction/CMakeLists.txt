set(MODULE_NAME "MLReconstruction")

if(NOT ${TF_FOUND})
  message(WARNING "No TensorFlow found. " ${MODULE_NAME}
                  " will not be build.")
  return()
endif()

message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC ${TF_INCLUDE_DIR})

target_link_libraries( ${MODULE_NAME} PUBLIC  Utilities DataFormats ${ROOT_LIBRARIES} ${TF_LIBRARIES})

reco_install_targets(${MODULE_NAME})    
install(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX})





