if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds are not allowed. mkdir build && cd build, then run cmake!\n"
  )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)

include(GNUInstallDirs)

include(FindPackageHandleStandardArgs)

find_program(CCACHE_EXECUTABLE ccache)
find_package_handle_standard_args(ccache DEFAULT_MSG CCACHE_EXECUTABLE)
if(CCACHE_EXECUTABLE)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()
mark_as_advanced(CCACHE_EXECUTABLE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Choose the type of build, options are: Debug, Release (default), RelWithDebInfo and MinSizeRel."
        FORCE)
endif()
