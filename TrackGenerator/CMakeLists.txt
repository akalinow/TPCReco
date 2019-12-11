set(MODULE_NAME "TrackGenerator")

# Finding Boost
find_package(Boost 1.45.0) 
if(Boost_FOUND)

find_package(SimUtils REQUIRED)
if(SimUtils_FOUND)
MESSAGE( STATUS "SimUtils found")
endif()
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
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
include_directories(/home/mfila/data/SimUtils/install/include)
#link_directories(/home/mfila/data/SimUtils/install/lib)
###############################################################################
## target definitions #########################################################
###############################################################################
# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#
add_library(${MODULE_NAME} SHARED ${sources})
target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} SimUtils)
add_executable(LineGenerator_test bin/LineGenerator_test.cpp)
add_executable(FromGeantGenerator_test bin/FromGeantGenerator_test.cpp)
add_executable(FromTransportGenerator_test bin/FromTransportGenerator_test.cpp)
# Compiler flags.
#
target_compile_options(LineGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(FromGeantGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(FromTransportGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
# Linker flags
#


target_link_libraries(LineGenerator_test PUBLIC ${MODULE_NAME} DataFormats Utilities)
target_link_libraries(FromGeantGenerator_test PUBLIC ${MODULE_NAME}  DataFormats Utilities)
target_link_libraries(FromTransportGenerator_test PUBLIC ${MODULE_NAME} SimUtils DataFormats Utilities )
# All install commands get the same destination. this allows us to use paths
# relative to the executable.



install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(TARGETS LineGenerator_test RUNTIME DESTINATION bin)
install(TARGETS FromGeantGenerator_test RUNTIME DESTINATION bin)
install(TARGETS FromTransportGenerator_test RUNTIME DESTINATION bin)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/config/> DESTINATION config)
else()
    MESSAGE( WARNING "No libboost found. " ${MODULE_NAME} " will not be build.")
endif()