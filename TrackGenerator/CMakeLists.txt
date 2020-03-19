set(MODULE_NAME "TrackGenerator")

# Finding Boost
find_package(Boost 1.45.0) 
if(Boost_FOUND)

# Adding external project
include(ExternalProject)
ExternalProject_Add(Sim_Utils SOURCE_DIR ${CMAKE_SOURCE_DIR}/TrackGenerator/deps/SimUtils CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR})
include_directories(${PROJECT_BINARY_DIR}/include)
link_directories(${PROJECT_BINARY_DIR}/lib)

MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variable
# `sources`
#

#file(GLOB_RECURSE sources       ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/*.cpp)
###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${Boost_INCLUDE_DIR})
include_directories(${ROOTINCDIR})
include_directories(${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${CMAKE_PREFIX_PATH}/include)

ROOT_GENERATE_DICTIONARY(G__${MODULE_NAME} ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/AbstractGenerator.h LINKDEF ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/RootDictLinkDef.h)
#link_directories(/home/mfila/data/SimUtils/install/lib)
###############################################################################
## target definitions #########################################################
###############################################################################
# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#

#add_library(${MODULE_NAME} SHARED ${sources}  G__${MODULE_NAME}.cxx)
add_library(${MODULE_NAME} SHARED 
${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/AbstractGenerator.cpp
${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/FromTransportGenerator.cpp
${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/LineGenerator.cpp
G__${MODULE_NAME}.cxx)


add_executable(EventGenerator bin/EventGenerator.cpp)
add_executable(SimpleGenerator bin/SimpleGenerator.cpp)
#add_executable(LineGenerator_test bin/LineGenerator_test.cpp)
#add_executable(FromGeantGenerator_test bin/FromGeantGenerator_test.cpp)
#add_executable(FromTransportGenerator_test bin/FromTransportGenerator_test.cpp)
# Compiler flags.
#
add_dependencies(${MODULE_NAME} Sim_Utils)
add_dependencies(EventGenerator Sim_Utils)
target_compile_options(EventGenerator PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(SimpleGenerator PUBLIC ${CMAKE_ROOT_CFLAGS})
#target_compile_options(LineGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
#target_compile_options(FromGeantGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
#target_compile_options(FromTransportGenerator_test PUBLIC ${CMAKE_ROOT_CFLAGS})
# Linker flags
#
target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} SimUtils)
target_link_libraries(EventGenerator PUBLIC ${MODULE_NAME} SimUtils DataFormats Utilities )
target_link_libraries(SimpleGenerator PUBLIC ${MODULE_NAME} DataFormats Utilities)
#target_link_libraries(LineGenerator_test PUBLIC ${MODULE_NAME} DataFormats Utilities)
#target_link_libraries(FromGeantGenerator_test PUBLIC ${MODULE_NAME}  DataFormats Utilities)
#target_link_libraries(FromTransportGenerator_test PUBLIC ${MODULE_NAME} SimUtils DataFormats Utilities )
# All install commands get the same destination. this allows us to use paths
# relative to the executable.



install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}_rdict.pcm DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}.rootmap DESTINATION lib)
install(TARGETS EventGenerator RUNTIME DESTINATION bin)
install(TARGETS SimpleGenerator RUNTIME DESTINATION bin)
#install(TARGETS LineGenerator_test RUNTIME DESTINATION bin)
#install(TARGETS FromGeantGenerator_test RUNTIME DESTINATION bin)
#install(TARGETS FromTransportGenerator_test RUNTIME DESTINATION bin)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/config/> DESTINATION config)

else()
    MESSAGE( WARNING "No libboost found. " ${MODULE_NAME} " will not be build.")
endif()