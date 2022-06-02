set(MODULE_NAME "Analysis")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )
set(TPCRECO_LIBRARIES_LOCAL ${TPCRECO_LIBRARIES})
set(TPCRECO_LIBRARIES ${TPCRECO_LIBRARIES} ${TPCRECO_PREFIX}${MODULE_NAME}  PARENT_SCOPE)

###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variable
# `sources`
#
file(GLOB_RECURSE sources       ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/*.cpp)

###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/GrawToROOT/include)
include_directories(${PROJECT_SOURCE_DIR}/GUI/include)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
###############################################################################
## target definitions #########################################################
###############################################################################
add_library(${MODULE_NAME} SHARED ${sources})
add_library(${TPCRECO_PREFIX}${MODULE_NAME} ALIAS ${MODULE_NAME})
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

add_executable(doNothing  bin/doNothing.cpp)
add_executable(makeTrackTree  bin/makeTrackTree.cpp)
add_executable(recoEventsAnalysis  bin/recoEventsAnalysis.cpp)
add_executable(recoEventsComparison  bin/recoEventsComparison.cpp)

# Compiler flags.
#
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(doNothing PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(makeTrackTree PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(recoEventsAnalysis PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(recoEventsComparison PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(${MODULE_NAME} ${TPCRECO_LIBRARIES_LOCAL} ${ROOT_LIBRARIES})
target_link_libraries(doNothing PUBLIC ${TPCRECO_LIBRARIES} ${Boost_LIBRARIES} ${MODULE_NAME})
target_link_libraries(makeTrackTree PUBLIC ${TPCRECO_LIBRARIES} ${Boost_LIBRARIES} ${MODULE_NAME})
target_link_libraries(recoEventsAnalysis PUBLIC  ${TPCRECO_LIBRARIES} ${Boost_LIBRARIES} ${MODULE_NAME})
target_link_libraries(recoEventsComparison PUBLIC ${TPCRECO_LIBRARIES} ${Boost_LIBRARIES} ${MODULE_NAME})

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(TARGETS doNothing RUNTIME DESTINATION bin)
install(TARGETS makeTrackTree RUNTIME DESTINATION bin)
install(TARGETS recoEventsAnalysis RUNTIME DESTINATION bin)
install(TARGETS recoEventsComparison RUNTIME DESTINATION bin)

install(PROGRAMS $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/python/> DESTINATION python)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/test/> DESTINATION test)
