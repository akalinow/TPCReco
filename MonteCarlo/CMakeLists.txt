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

configure_file(
  config/ModuleConfig.json
  ${CMAKE_CURRENT_BINARY_DIR}/config/montecarlo_ModuleConfig.json @ONLY)
configure_file(
  config/EventGenerator.json
  ${CMAKE_CURRENT_BINARY_DIR}/config/montecarlo_EventGenerator.json @ONLY)
configure_file(
  config/ModuleConfigGun.json
  ${CMAKE_CURRENT_BINARY_DIR}/config/montecarlo_ModuleConfigGun.json @ONLY)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/config
        DESTINATION ${CMAKE_INSTALL_PREFIX}/config)
