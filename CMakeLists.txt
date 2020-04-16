cmake_minimum_required(VERSION 3.5) # setting this is required
set(This TPCSoft)
project(${This} CXX)                   # this sets the project name
set(Tests_flag False)
set(GET_flag False)

###############################################################################
## dependencies ###############################################################
###############################################################################

#ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
list(APPEND CMAKE_MODULE_PATH $ENV{ROOTSYS}/etc/cmake)
find_package(ROOT REQUIRED COMPONENTS Physics HistPainter)
include(${ROOT_USE_FILE})

MESSAGE(STATUS "CMAKE_PREFIX_PATH: " ${CMAKE_PREFIX_PATH})
MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
MESSAGE(STATUS "ROOTSYS: " $ENV{ROOTSYS})
MESSAGE(STATUS "ROOT_INCLUDE_DIRS: " ${ROOT_INCLUDE_DIRS})
MESSAGE(STATUS "ROOT_LIBRARIES: " ${ROOT_LIBRARIES})
MESSAGE(STATUS "ROOT_USE_FILE: " ${ROOT_USE_FILE})

if (GET_flag) # temporary flag to get rid of compilation errors
	#GET software presence
	set(GET_VERSION_DIR $ENV{GET_DIR}/GetSoftware_$ENV{GET_RELEASE})
	if(EXISTS ${GET_VERSION_DIR})
		set(GET_LIBRARIES ${GET_VERSION_DIR}/lib/libget-root.so ${GET_VERSION_DIR}/lib/libcobo-frame-graw2frame.so ${GET_VERSION_DIR}/lib/libMultiFrame.so)
		set(GET_CXX_FLAGS -Wno-deprecated-declarations -DUTL_LOG_LEVEL=10)	
		add_definitions(-DWITH_GET)
		set(GRAW_TO_ROOT_LIBRARY GrawToROOT)
		include_directories(${GET_VERSION_DIR}/include)
	else()
		MESSAGE(WARNING "No GET software found. ")
		set(GET_LIBRARIES "")
		set(GET_CXX_FLAGS "")
		set(GRAW_TO_ROOT_LIBRARY "")
	endif()  
	MESSAGE( STATUS "GET_VERSION_DIR:       " ${GET_VERSION_DIR})
	MESSAGE( STATUS "GET libraries:         " ${GET_LIBRARIES} )
	MESSAGE( STATUS "GET compile flags:     " ${GET_CXX_FLAGS} )
endif()
#####################################################################

###############################################################################
## packaging ##################################################################
###############################################################################
set(CMAKE_INSTALL_PREFIX $ENV{PWD}) # install in the current directory
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX:         " ${CMAKE_INSTALL_PREFIX} )

# This is basically a repeat of the file copy instruction that copies the
# resources in the build directory, but here we tell cmake that we want it
# in the package.
#

if(NOT IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/resources) # needs full path!
  install(DIRECTORY resources DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
  MESSAGE(STATUS "RESOURCES dir already exists in the installation dir")
endif()

set(CMAKE_CXX_FLAGS "${ROOT_CXX_FLAGS} -Wall -O3 -std=c++14 -fopenmp -pthread")

#Set RPATH to install dir, so user do not have to set the LD_LIBRARY_PATH
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

#Framework packages
if(Tests_flag)
	add_subdirectory(googletest)
	add_subdirectory(DataFormatsTests)
endif()
add_subdirectory(GUI)
add_subdirectory(GrawToROOT)
add_subdirectory(Utilities)
add_subdirectory(Reconstruction)
add_subdirectory(TrackGenerator)
add_subdirectory(DataFormats)

# Now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here.
set(CPACK_PACKAGE_NAME "${This}")
set(CPACK_PACKAGE_VERSION "1.0.0")

# We don't want to split our program up into several incomplete pieces.
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
