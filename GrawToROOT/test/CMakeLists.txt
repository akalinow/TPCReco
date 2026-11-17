set(TEST_DEFINITIONS
    TESTDATA_DIR="${PROJECT_SOURCE_DIR}/EventSources/test/testData/"
    RESOURCES_DIR="${PROJECT_SOURCE_DIR}/resources/")

add_unit_test(GrawFile_tst GrawToROOT Boost::filesystem)
target_compile_definitions(GrawFile_tst PRIVATE ${TEST_DEFINITIONS})

# Comparison with the GET software reader, if available
if(${GET_FOUND})
  find_library(GET_ROOT_LIBRARY get-root HINTS ${GET_INCLUDE_DIR}/../lib)
  find_package(Qt5 REQUIRED COMPONENTS Core)  # needed by the GET libraries
  add_unit_test(GrawFileVsGET_tst GrawToROOT GET::cobo-frame-graw2frame GET::MultiFrame
                ${GET_ROOT_LIBRARY} Qt5::Core)
  target_compile_definitions(GrawFileVsGET_tst PRIVATE ${TEST_DEFINITIONS})
endif()
