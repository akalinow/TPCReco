set(MODULE_NAME "EventGenerator")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_link_libraries(${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_MATHMORE_LIBRARY}
                                            GenVector Utilities UtilsMC)
if(TARGET ROOT::MathMore)
  target_compile_definitions(${MODULE_NAME} PRIVATE HAS_ROOT_MATHMORE)
endif()
reco_install_targets(${MODULE_NAME})

reco_add_test_subdirectory(test)
