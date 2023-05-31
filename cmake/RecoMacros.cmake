set(TPCRECO_LIBPREFIX "libTPC")
set(TPCRECO_PREFIX "TPC")

function(reco_set_compile_options NAME)
  target_compile_options(${NAME} PRIVATE $<$<CONFIG:RELEASE>:-O3 -DNDEBUG>
                                         $<$<CONFIG:DEBUG>:-g3> -Wall -Werror)
endfunction()

function(reco_add_library NAME)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs EXTRA_SOURCES)
  cmake_parse_arguments(RECO_ADD_LIBRARY "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  file(GLOB_RECURSE SOURCES "src/*")
  file(GLOB_RECURSE HEADERS "include/*")
  add_library(${NAME} SHARED ${SOURCES} ${RECO_ADD_LIBRARY_EXTRA_SOURCES})
  add_library(TPCReco::${NAME} ALIAS ${NAME})
  target_include_directories(
    ${NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                   $<INSTALL_INTERFACE:include/>)
  set_target_properties(${NAME} PROPERTIES PREFIX "${TPCRECO_LIBPREFIX}")
  reco_set_compile_options(${NAME})

  if(NOT "${CMAKE_VERSION}" VERSION_LESS "3.8")
    target_compile_features(${NAME} PUBLIC cxx_std_14)
  else()
    target_compile_features(${NAME} PUBLIC cxx_auto_type cxx_generic_lambdas)
  endif()
endfunction()

function(reco_add_executable NAME)
  add_executable(${NAME} ${ARGN})
  reco_set_compile_options(${NAME})
endfunction()

function(reco_install_targets)
  install(
    TARGETS ${ARGV}
    EXPORT TPCRecoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY include/TPCReco DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endfunction()

function(reco_install_root_dict NAME)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${TPCRECO_LIBPREFIX}${NAME}_rdict.pcm
          ${CMAKE_CURRENT_BINARY_DIR}/${TPCRECO_LIBPREFIX}${NAME}.rootmap
    DESTINATION ${CMAKE_INSTALL_LIBDIR})
endfunction()
