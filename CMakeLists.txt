cmake_minimum_required(VERSION 3.4) # setting this is required
project(
  TPCReco
  LANGUAGES CXX
  VERSION 0.7.0)

if("${CMAKE_VERSION}" VERSION_LESS "3.8")
  set(CMAKE_CXX_STANDARD 14)
endif()

if(NOT CMAKE_MODULE_PATH)
  set(CMAKE_MODULE_PATH)
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(Utils)
include(Tests)
include(RecoMacros)

find_package(Boost REQUIRED COMPONENTS program_options filesystem date_time)

find_package(ROOT 6.08 REQUIRED COMPONENTS Core Physics HistPainter RIO
                                           GenVector Gui)
message(STATUS "ROOT version: ${ROOT_VERSION}")
include(${ROOT_USE_FILE})
separate_arguments(ROOT_EXE_LINKER_FLAGS)

find_package(GET COMPONENTS MultiFrame cobo-frame-graw2frame)

# default install in current directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "$ENV{PWD}"
      CACHE PATH "..." FORCE)
endif()
message(STATUS "CMAKE_INSTALL_PREFIX:\t${CMAKE_INSTALL_PREFIX}")

add_library(Resources INTERFACE)
set(TPCRECO_RESOURCE_DIR
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>/resources/"
)
target_compile_definitions(
  Resources INTERFACE TPCRECO_RESOURCE_DIR=\"${TPCRECO_RESOURCE_DIR}\")

option(BUILD_GEANT_MODULE "Build Geant4 simulation" OFF)

<<<<<<< HEAD
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

set(CMAKE_CXX_FLAGS "${ROOT_CXX_FLAGS} -std=c++17 -Wall -Werror -O3 -pthread -Wno-register -Wno-error=deprecated-declarations")

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
=======
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
add_subdirectory(Utilities)
add_subdirectory(DataFormats)
add_subdirectory(GrawToROOT)
add_subdirectory(Reconstruction)
add_subdirectory(Analysis)
add_subdirectory(GUI)
add_subdirectory(Simulation)
add_subdirectory(MonteCarlo)

if(NOT IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/resources)
  install(DIRECTORY resources DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

install(
  EXPORT TPCRecoTargets
  FILE TPCRecoTargets.cmake
  NAMESPACE TPCReco::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TPCReco)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/TPCRecoConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/TPCRecoConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TPCReco)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/TPCRecoConfigVersion.cmake"
  VERSION "${TPCReco_VERSION_MAJOR}.${TPCReco_VERSION_MINOR}"
  COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TPCRecoConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/TPCRecoConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TPCReco)

add_subdirectory(package)
