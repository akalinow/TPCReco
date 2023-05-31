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

<<<<<<< HEAD
###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/GrawToROOT/include)
include_directories(${PROJECT_SOURCE_DIR}/TrackGenerator/include)
include_directories(${PROJECT_SOURCE_DIR}/GUI/include)
###############################################################################
## target definitions #########################################################
###############################################################################
# ROOT dictionary for persistens classes
ROOT_GENERATE_DICTIONARY(G__${MODULE_NAME}
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/CommonDefinitions.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/PEventTPC.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/EventTPC.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/GeometryTPC.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/SigClusterTPC.h 
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/Hit2D.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/Track3D.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/TrackSegment3D.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/TrackSegment2D.h  
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/MultiKey.h 
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/EventRaw.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/EventInfo.h
  LINKDEF ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/LinkDef.h)
=======
reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES}
                                            ${ROOT_EXE_LINKER_FLAGS} Utilities)
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

reco_install_targets(${MODULE_NAME})
reco_install_root_dict(${MODULE_NAME})

reco_add_test_subdirectory(test)
