set(MODULE_NAME "EventGenerator")
MESSAGE(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} MathMore GenVector Utilities UtilsMC)
reco_install_targets(${MODULE_NAME})

reco_add_test_subdirectory(test)



