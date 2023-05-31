set(MODULE_NAME "Simulation")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} DataFormats
                        Utilities Reconstruction)

reco_install_targets(${MODULE_NAME})
