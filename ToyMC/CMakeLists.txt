set(MODULE_NAME "ToyMC")
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
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
###############################################################################
## target definitions #########################################################
###############################################################################
# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#
add_executable(UtilsTPC_test test/UtilsTPC_test.cpp)
add_executable(GeometryTPC_test test/GeometryTPC_test.cpp)
add_executable(TrackSegmentTPC_test test/TrackSegmentTPC_test.cpp)
# Compiler flags.
#
target_compile_options(UtilsTPC_test PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(GeometryTPC_test PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(TrackSegmentTPC_test PUBLIC ${CMAKE_ROOT_CFLAGS})

# Linker flags
#
target_link_libraries(UtilsTPC_test  PUBLIC DataFormats Utilities)
target_link_libraries(GeometryTPC_test PUBLIC DataFormats Utilities)
target_link_libraries(TrackSegmentTPC_test PUBLIC DataFormats Utilities)

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS UtilsTPC_test GeometryTPC_test TrackSegmentTPC_test RUNTIME DESTINATION bin)
install(DIRECTORY DESTINATION results)
