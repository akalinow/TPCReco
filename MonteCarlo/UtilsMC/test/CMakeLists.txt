file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(example_source ${sources})
  get_filename_component(example ${example_source} NAME_WE)
  add_unit_test(${example} UtilsMC)
endforeach(example_source ${sources})
