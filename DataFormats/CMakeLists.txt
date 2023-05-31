set(MODULE_NAME "DataFormats")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include/)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include/)
root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventTPC.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/GeometryTPC.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/SigClusterTPC.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/Hit2D.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/Track3D.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/TrackSegment3D.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/TrackSegment2D.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/MultiKey.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventRaw.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventInfo.h
  LINKDEF
  LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES}
                                            ${ROOT_EXE_LINKER_FLAGS} Utilities)
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

reco_install_targets(${MODULE_NAME})
reco_install_root_dict(${MODULE_NAME})

reco_add_test_subdirectory(test)