set(MODULE_NAME "Reconstruction")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_link_libraries(
  ${MODULE_NAME}
  PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} DataFormats Utilities
  PRIVATE Resources)

reco_install_targets(${MODULE_NAME})
install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_PREFIX})
