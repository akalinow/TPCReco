set(MODULE_NAME "MonteCarloModules")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

add_subdirectory(Generator)
add_subdirectory(DummyModule)
add_subdirectory(EventFileExporter)
add_subdirectory(TPCDigitizerRandom)
add_subdirectory(TPCDigitizerSRC)
add_subdirectory(Track3DBuilder)
add_subdirectory(TrackTruncator)
add_subdirectory(TriggerSimulator)
add_subdirectory(ToyIonizationSimulator)

if(BUILD_GEANT_MODULE)
  find_package(Geant4 REQUIRED ui_all vis_all)
  find_package(cadmesh REQUIRED)
  add_subdirectory(GeantSim)
  include_directories(${Geant4_INCLUDE_DIRS})
  include_directories(${CADMESH_INCLUDE_DIRS})
  include_directories(GeantSim/include)

  include(ExternalProject)
  ExternalProject_Add(
    geant_model
    URL https://cernbox.cern.ch/remote.php/dav/public-files/DWjBUbnsCuwE15M/elitpc_STL_model_20180302.tar.gz
    SOURCE_DIR "${CMAKE_INSTALL_PREFIX}/resources/Geant_STL_model"
    DOWNLOAD_DIR "${CMAKE_INSTALL_PREFIX}/resources/Geant_STL_model"
    URL_HASH MD5=0c2999ee658f0dec54f729ed889eff01
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND "")
endif()

set(module_sources
    ${generator_sources}
    ${dummy_sources}
    ${event_file_exporter_sources}
    ${geant_sim_sources}
    ${tpc_digitizer_random_sources}
    ${tpc_digitizer_src_sources}
    ${build_track3d_sources}
    ${track_truncator_sources}
    ${trigger_simulator_sources}
    ${toy_ionization_simulator_sources})

reco_add_library(${MODULE_NAME} EXTRA_SOURCES ${module_sources})

if(BUILD_GEANT_MODULE)
  add_dependencies(${MODULE_NAME} geant_model)
endif()
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

target_link_libraries(
  ${MODULE_NAME}
  ${ROOT_LIBRARIES}
  GenVector
  Utilities
  UtilsMC
  EventGenerator
  Reconstruction)
if(BUILD_GEANT_MODULE)
  target_link_libraries(${MODULE_NAME} ${Geant4_LIBRARIES} ${cadmesh_LIBRARIES})
endif()

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
