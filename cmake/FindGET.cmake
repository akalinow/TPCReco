# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindGET
-------

Finds the GET library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``GET::MultiFrame``
  The MultiFrame library
``GET::cobo-frame-graw2frame``
  The cobo-frame-graw2frame library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``GET_FOUND``
  True if the system has the GET library.
``GET_VERSION``
  The version of the GET library which was found.
``GET_INCLUDE_DIRS``
  Include directories needed to use GET.
``GET_LIBRARIES``
  Libraries needed to link to GET.
``GET_MultiFrame_LIBRARY``
  Libraries needed to link to GET::MultiFrame.
``GET_cobo-frame-graw2frame_LIBRARY``
  Libraries needed to link to GET::cobo-frame-graw2frame.

#]=======================================================================]

set(GET_VERSION_DIR $ENV{GET_DIR}/GetSoftware_bin/$ENV{GET_RELEASE})

find_path(
  GET_INCLUDE_DIR
  NAMES get/GDataFrame.h
  PATHS ${GET_VERSION_DIR}
  PATH_SUFFIXES include)

set(GET_LIBRARIES "MultiFrame" "cobo-frame-graw2frame")

foreach(comp IN LISTS GET_LIBRARIES)
  find_library(
    GET_${comp}_LIBRARY
    NAMES ${comp}
    PATHS ${GET_VERSION_DIR}
    PATH_SUFFIXES lib)
  set(GET_${comp}_FOUND TRUE)
endforeach()

set(GET_VERSION $ENV{GET_RELEASE})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  GET HANDLE_COMPONENTS
  FOUND_VAR GET_FOUND
  REQUIRED_VARS GET_LIBRARIES GET_INCLUDE_DIR
  VERSION_VAR GET_VERSION)

foreach(comp IN LISTS GET_FIND_COMPONENTS)
  add_library(GET::${comp} UNKNOWN IMPORTED)
  set_target_properties(
    GET::${comp}
    PROPERTIES
      IMPORTED_LOCATION "${GET_${comp}_LIBRARY}"
      INTERFACE_COMPILE_OPTIONS
      "-Wno-deprecated-declarations;-DUTL_LOG_LEVEL=10;-DWITH_GET;-Wno-register"
      INTERFACE_INCLUDE_DIRECTORIES "${GET_INCLUDE_DIR}")
endforeach()

foreach(comp IN LISTS GET_FIND_COMPONENTS)
  mark_as_advanced(GET_${comp}_LIBRARY)
endforeach()

mark_as_advanced(GET_INCLUDE_DIR)
