set(MODULE_NAME "Analysis")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )

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
add_executable(analysis_IFJ_VdG  bin/analysis_IFJ_VdG.cpp)
add_executable(makeTrackTree  bin/makeTrackTree.cpp)

# Compiler flags.
#
target_compile_options(analysis_IFJ_VdG PUBLIC ${CMAKE_ROOT_CFLAGS})
target_link_libraries(analysis_IFJ_VdG PUBLIC DataFormats Utilities Reconstruction GUI ${Boost_LIBRARIES})

target_compile_options(makeTrackTree PUBLIC ${CMAKE_ROOT_CFLAGS})
target_link_libraries(makeTrackTree PUBLIC DataFormats ${GRAW_TO_ROOT_LIBRARY} Utilities Reconstruction GUI ${Boost_LIBRARIES})

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS analysis_IFJ_VdG RUNTIME DESTINATION bin)
install(TARGETS makeTrackTree RUNTIME DESTINATION bin)
install(PROGRAMS $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/python/> DESTINATION python)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/test/> DESTINATION test)