option(BUILD_TEST "build tests" OFF)

function(reco_add_test_subdirectory SUBDIR)
  if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TEST)
    add_subdirectory(${SUBDIR})
  endif()
endfunction()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TEST)

  if(CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
  else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
  endif()

  include(DownloadProject)
  download_project(
    PROJ
    googletest
    GIT_REPOSITORY
    https://github.com/google/googletest.git
    GIT_TAG
    main
    ${UPDATE_DISCONNECTED_IF_AVAILABLE})

  set(INSTALL_GTEST OFF)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
  include(CTest)
  enable_testing()

  mark_as_advanced(
    BUILD_GMOCK
    BUILD_GTEST
    BUILD_SHARED_LIBS
    INSTALL_GTEST
    gmock_build_tests
    gtest_build_samples
    gtest_build_tests
    gtest_disable_pthreads
    gtest_force_shared_crt
    gtest_hide_internal_symbols)

  macro(add_unit_test NAME)
    add_executable(${NAME} ${NAME}.cpp)
    foreach(arg IN ITEMS ${ARGN})
      target_link_libraries(${NAME} PRIVATE ${arg})
    endforeach()
    target_link_libraries(${NAME} PRIVATE gmock gtest_main)
    add_test(NAME ${NAME} COMMAND ${NAME}
                                  "--gtest_output=xml:test_${NAME}_report.xml")
  endmacro()

endif()
