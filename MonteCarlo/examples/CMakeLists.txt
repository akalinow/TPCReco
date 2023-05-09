set(MODULE_NAME "MonteCarloExamples")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(example_source ${sources})
  get_filename_component(example ${example_source} NAME_WE)
  reco_add_executable(${example} ${example_source})
  target_link_libraries(
    ${example}
    PRIVATE ${ROOT_LIBRARIES}
            Utilities
            DataFormats
            UtilsMC
            EventGenerator
            MonteCarloModules
            ${Boost_LIBRARIES})
  install(TARGETS ${example} RUNTIME DESTINATION bin/examples)
endforeach(example_source ${sources})
