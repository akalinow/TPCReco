cmake_minimum_required(VERSION 3.0) # setting this is required
project(TPCSoft)                   # this sets the project name

###############################################################################
## dependencies ###############################################################
###############################################################################

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
MESSAGE( STATUS "ROOT_USE_FILE:             " ${ROOT_USE_FILE})

###############################################################################
## packaging ##################################################################
###############################################################################
set(CMAKE_INSTALL_PREFIX "$ENV{PWD}") # install in the current directory
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

set( CMAKE_CXX_FLAGS "${ROOT_CXX_FLAGS} -Wall -O3 -std=c++14 -fopenmp")

#Set RPATH to install dir, so user do not have to set the LD_LIBRARY_PATH
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)


#Framework packages
add_subdirectory(GUI)
add_subdirectory(GrawToROOT)
add_subdirectory(Reconstruction)
add_subdirectory(DataFormats)

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
