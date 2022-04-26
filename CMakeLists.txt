cmake_minimum_required(VERSION 3.0) # setting this is required
project(TPCSoft)                   # this sets the project name

###############################################################################
## dependencies ###############################################################
###############################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(Tests)

#ROOT 
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
list(APPEND CMAKE_MODULE_PATH $ENV{ROOTSYS}/etc/cmake)
find_package(ROOT REQUIRED COMPONENTS Physics HistPainter)
include(${ROOT_USE_FILE})

MESSAGE( STATUS "CMAKE_PREFIX_PATH:         " ${CMAKE_PREFIX_PATH} )
MESSAGE( STATUS "CMAKE_MODULE_PATH:         " ${CMAKE_MODULE_PATH} )
MESSAGE( STATUS "ROOTSYS:                   " $ENV{ROOTSYS} )
MESSAGE( STATUS "ROOT_INCLUDE_DIRS:         " ${ROOT_INCLUDE_DIRS} )
MESSAGE( STATUS "ROOT_LIBRARIES:            " ${ROOT_LIBRARIES} )
MESSAGE( STATUS "ROOT_CFLAGS:               " ${CMAKE_ROOT_CFLAGS})
MESSAGE( STATUS "ROOT_USE_FILE:             " ${ROOT_USE_FILE})

MESSAGE( STATUS "CMAKE_CXX_FLAGS:             " ${CMAKE_CXX_FLAGS})


#GET software presence
set(GET_VERSION_DIR $ENV{GET_DIR}/GetSoftware_bin/$ENV{GET_RELEASE})
if(EXISTS ${GET_VERSION_DIR})
  set(GET_LIBRARIES ${GET_VERSION_DIR}/lib/libget-root.so ${GET_VERSION_DIR}/lib/libcobo-frame-graw2frame.so ${GET_VERSION_DIR}/lib/libMultiFrame.so)
  set(GET_CXX_FLAGS -Wno-deprecated-declarations -DUTL_LOG_LEVEL=10)
  add_definitions(-DWITH_GET)
  set(GRAW_TO_ROOT_LIBRARY GrawToROOT)
  include_directories(${GET_VERSION_DIR}/include)
else()
    MESSAGE(WARNING "No GET software found. GET dependent packages will not be built.")
    set(GET_LIBRARIES "")
    set(GET_CXX_FLAGS "")
    set(GRAW_TO_ROOT_LIBRARY "")
endif()
  
MESSAGE( STATUS "GET_VERSION_DIR:       " ${GET_VERSION_DIR})
MESSAGE( STATUS "GET libraries:         " ${GET_LIBRARIES} )
MESSAGE( STATUS "GET includes:          " ${GET_VERSION_DIR}/include )
MESSAGE( STATUS "GET compile flags:     " ${GET_CXX_FLAGS} )
#####################################################################
find_package(Boost 1.45.0 COMPONENTS program_options REQUIRED)
if(Boost_FOUND)
  MESSAGE(STATUS "Boost verion found:" ${Boost_VERSION_STRING})
else()
  MESSAGE(FATAL_ERROR "No Boost found.")
endif()  
###############################################################################
## packaging ##################################################################
###############################################################################

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)  # install in the current directory
  set(CMAKE_INSTALL_PREFIX
      "$ENV{PWD}"
      CACHE PATH "..." FORCE)
endif()


MESSAGE( STATUS "CMAKE_INSTALL_PREFIX:         " ${CMAKE_INSTALL_PREFIX} )

# This is basically a repeat of the file copy instruction that copies the
# resources in the build directory, but here we tell cmake that we want it
# in the package.
#
if(NOT IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/resources) # needs full path!
  install(DIRECTORY resources DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
  MESSAGE( STATUS "RESOURCES dir already exists in the installation dir" )
endif()
if(NOT IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/tools) # needs full path!
  install(DIRECTORY tools DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
  MESSAGE( STATUS "AUXILIARY TOOLS dir already exists in the installation dir" )
endif()

set(CMAKE_CXX_FLAGS "${ROOT_CXX_FLAGS} -Wall -Werror -O3 -std=c++17 -pthread")
MESSAGE( STATUS "CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS})

option(DEBUG "Define DEBUG variable" OFF)
if(DEBUG)
  MESSAGE( STATUS "DEBUG enabled" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()
# Set default inotify message inplementation:
option(DIRECTORYWATCH_ONE_MESSAGE_DISABLE "Define DIRECTORYWATCH_ONE_MESSAGE_DISABLE variable" OFF)
if(DIRECTORYWATCH_ONE_MESSAGE_DISABLE)
  MESSAGE( STATUS "DirectoryWatch single message disabled (i.e. one Message per modified file)" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDIRECTORYWATCH_ONE_MESSAGE_DISABLE")
else()
  MESSAGE( STATUS "DirectoryWatch single message enabled (i.e. single Message per all modified files)" )
endif()  
# Enable/disable searching next file with N+1 index (default=ON)
option(EVENTSOURCEGRAW_NEXT_FILE_DISABLE "Define EVENTSOURCEGRAW_NEXT_FILE_DISABLE variable" ON)
if(EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
  MESSAGE( STATUS "EventSourceGRAW automatic next file index disabled" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEVENTSOURCEGRAW_NEXT_FILE_DISABLE")
else()
  MESSAGE( STATUS "EventSourceGRAW automatic next file index enabled" )
endif()  
  
#Set RPATH to install dir, so user do not have to set the LD_LIBRARY_PATH
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

#Framework packages
add_subdirectory(DataFormats)
add_subdirectory(GrawToROOT) 
add_subdirectory(Utilities)
add_subdirectory(Reconstruction)
add_subdirectory(GUI)
add_subdirectory(Analysis)
add_subdirectory(ToyMC)
option(BUILD_MC "build MC tools" OFF)
if(BUILD_MC)
  add_subdirectory(TrackGenerator)
endif()
# Now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here.
set(CPACK_PACKAGE_NAME "TPCSoft")
set(CPACK_PACKAGE_VERSION "1.0.0")

# We don't want to split our program up into several incomplete pieces.
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
