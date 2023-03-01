set(MODULE_NAME "MonteCarloModules")

include_directories(${ROOTINCDIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/UtilsMC/include)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/EventGenerator/include)



ADD_SUBDIRECTORY(Generator)
ADD_SUBDIRECTORY(DummyModule)
ADD_SUBDIRECTORY(EventFileExporter)
ADD_SUBDIRECTORY(TPCDigitizerRandom)
ADD_SUBDIRECTORY(TPCDigitizerSRC)
ADD_SUBDIRECTORY(Track3DBuilder)

IF (BUILD_GEANT_MODULE)
    find_package(Geant4 REQUIRED ui_all vis_all)
    find_package(cadmesh REQUIRED)
    ADD_SUBDIRECTORY(GeantSim)
    include_directories(${Geant4_INCLUDE_DIRS})
    include_directories(${CADMESH_INCLUDE_DIRS})
    include_directories(GeantSim/include)
ENDIF ()

SET(module_sources
        ${generator_sources}
        ${dummy_sources}
        ${event_file_exporter_sources}
        ${geant_sim_sources}
        ${tpc_digitizer_random_sources}
        ${tpc_digitizer_src_sources}
        ${build_track3d_sources}
        )





ADD_LIBRARY(${MODULE_NAME} SHARED
        ${module_sources}
        )

set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} GenVector Utilities UtilsMC EventGenerator Reconstruction)
IF (BUILD_GEANT_MODULE)
    target_link_libraries(${MODULE_NAME} ${Geant4_LIBRARIES} ${cadmesh_LIBRARIES})
ENDIF ()



target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)