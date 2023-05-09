add_subdirectory(UtilsMC)
add_subdirectory(EventGenerator)
add_subdirectory(examples)
add_subdirectory(Modules)

reco_add_executable(mcRunController bin/mcRunController.cpp)

target_link_libraries(
  mcRunController
  PRIVATE ${ROOT_LIBRARIES}
          Utilities
          DataFormats
          UtilsMC
          EventGenerator
          MonteCarloModules
          ${Boost_LIBRARIES})

# install mcRunController 'by hand' here because there is no include/TPCReco
# directory required by the reco_install_targets macro
install(TARGETS mcRunController RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/ModuleConfig.json ${CMAKE_INSTALL_PREFIX}/config/montecarlo_ModuleConfig.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/EventGenerator.json ${CMAKE_INSTALL_PREFIX}/config/montecarlo_EventGenerator.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/ModuleConfigGun.json ${CMAKE_INSTALL_PREFIX}/config/montecarlo_ModuleConfigGun.json)
