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


set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
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
