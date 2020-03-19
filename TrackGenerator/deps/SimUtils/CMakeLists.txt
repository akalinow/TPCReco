cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(SimUtils)


#cmake magic to link libraries when installing
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB util_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB util_headers ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh)

find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

ROOT_GENERATE_DICTIONARY(SimUtilsDict ${CMAKE_CURRENT_SOURCE_DIR}/include/SimEvent.hh LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h)

#---Create a shared library with geneated dictionary
#add_library(SimEvent SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/SimEvent.cc SimEventDict.cxx)
add_library(SimUtils SHARED ${util_sources} ${util_headers} SimUtilsDict.cxx)
target_link_libraries(SimUtils ${ROOT_LIBRARIES})
target_include_directories(SimUtils PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:include>)

#---Create  a main program using the library

install(TARGETS SimUtils
		EXPORT SimUtils-targets	
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
       )

install(
      FILES
      ${CMAKE_CURRENT_BINARY_DIR}/libSimUtilsDict_rdict.pcm
      ${CMAKE_CURRENT_BINARY_DIR}/libSimUtilsDict.rootmap
      DESTINATION lib/)

install(EXPORT SimUtils-targets
    FILE SimUtilsConfig.cmake
    DESTINATION lib/cmake/SimUtils
)

install(FILES ${util_headers} DESTINATION include)