set(MODULE_NAME "MonteCarloExamples")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )

file(GLOB_RECURSE sources       ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/UtilsMC/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/UtilsMC/Modules/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/EventGenerator/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)


foreach( example_source ${sources} )
    get_filename_component(example ${example_source} NAME_WE)
    add_executable( ${example} ${example_source} )
    target_link_libraries( ${example} Utilities DataFormats UtilsMC EventGenerator MonteCarloModules ${Boost_LIBRARIES} )
    install(TARGETS ${example} RUNTIME DESTINATION bin/examples)
endforeach( example_source ${sources} )



