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

#Download strip response files
include(ExternalProject)
ExternalProject_Add(
  strip_response_model
  URL https://mycloud.fuw.edu.pl/index.php/s/x2DBMLei33aKTY3/download
  DOWNLOAD_NAME StripResponseModel.tar
  SOURCE_DIR StripResponseModel
  DOWNLOAD_DIR StripResponseModel
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  CONFIGURE_COMMAND "")

if(BUILD_GEANT_MODULE)
  find_package(Geant4 REQUIRED ui_all vis_all)
  find_package(cadmesh REQUIRED)
  add_subdirectory(GeantSim)
  include_directories(${Geant4_INCLUDE_DIRS})
  include_directories(${CADMESH_INCLUDE_DIRS})
  include_directories(GeantSim/include)

  include(ExternalProject)
  # Download the Geant STL model
  ExternalProject_Add(
    geant_model
    URL https://mycloud.fuw.edu.pl/index.php/s/ZfeoMXdX3Z5bikQ/download 
    DOWNLOAD_NAME elitpc_STL_model_20180302.tar.gz
    SOURCE_DIR Geant_STL_model
    DOWNLOAD_DIR Geant_STL_model
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
  add_dependencies(${MODULE_NAME} strip_response_model)
endif()

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} GenVector Utilities UtilsMC
                        EventGenerator Reconstruction)
if(BUILD_GEANT_MODULE)
  target_link_libraries(${MODULE_NAME} PUBLIC ${Geant4_LIBRARIES}
                                              ${cadmesh_LIBRARIES})
endif()

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

install(
    TARGETS ${MODULE_NAME}
    EXPORT TPCRecoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
