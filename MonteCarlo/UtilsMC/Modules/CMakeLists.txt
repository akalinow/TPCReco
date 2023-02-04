set(MODULE_NAME "MonteCarloModules")

ADD_SUBDIRECTORY(Generator)
ADD_SUBDIRECTORY(DummyModule)

file(GLOB_RECURSE fwk_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

SET (module_sources
        ${generator_sources}
        ${dummy_sources}
        ${fwk_sources}
        )

include_directories(${ROOTINCDIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/UtilsMC/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/EventGenerator/include)


ADD_LIBRARY (${MODULE_NAME} SHARED
        ${module_sources}
        )

set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} GenVector Utilities UtilsMC EventGenerator)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)