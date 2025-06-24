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

file(GLOB config_files "${CMAKE_CURRENT_SOURCE_DIR}/config/*.json")

foreach(config_file IN LISTS config_files)
  get_filename_component(config_filename ${config_file} NAME)
  configure_file(
    ${config_file}
    ${CMAKE_CURRENT_BINARY_DIR}/config/${config_filename} @ONLY)
endforeach()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/config
        DESTINATION ${CMAKE_INSTALL_PREFIX})

