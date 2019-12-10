set(MODULE_NAME "TrackGenerator")

# Finding Boost
find_package(Boost 1.45.0) 
if(Boost_FOUND)
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
add_library(${MODULE_NAME} SHARED ${sources})
target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES})
add_executable(LineGenerator_test bin/LineGenerator_test.cpp)
# Compiler flags.
#
target_compile_options(LineGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
# Linker flags
#
target_link_libraries(LineGenerator_test PUBLIC ${MODULE_NAME} DataFormats Utilities)
# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(TARGETS LineGenerator_test RUNTIME DESTINATION bin)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/config/> DESTINATION config)
else()
    MESSAGE( WARNING "No libboost found. " ${MODULE_NAME} " will not be build.")
endif()