add_subdirectory(UtilsMC)
add_subdirectory(EventGenerator)
add_subdirectory(examples)
add_subdirectory(Modules)

reco_add_executable(mcRunController bin/mcRunController.cpp)

target_link_libraries(
  mcRunController ${ROOT_LIBRARIES} Utilities DataFormats UtilsMC
  EventGenerator MonteCarloModules ${Boost_LIBRARIES})

#install mcRunController 'by hand' here because there is no include/TPCReco directory
#required by the reco_install_targets macro
install(TARGETS mcRunController RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})