set(MODULE_NAME "GUI")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include/)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/Analysis/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/GUI/include)
root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/MainFrame.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/SelectionBox.h
  LINKDEF
  LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)
reco_add_executable(tpcGUI bin/tpcGUI.cxx)
reco_add_executable(dumpProjectionHistos bin/dumpProjectionHistos.cpp)
reco_add_executable(dumpClusterHistos bin/dumpClusterHistos.cpp)
reco_add_executable(dumpRateHistos bin/dumpRateHistos.cpp)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} DataFormats 
                         EventSources Reconstruction Analysis
                         $<$<BOOL:${GET_FOUND}>:GrawToROOT>)
target_link_libraries(tpcGUI PRIVATE Boost::program_options ${MODULE_NAME})
target_link_libraries(dumpProjectionHistos PRIVATE ${MODULE_NAME})
target_link_libraries(dumpClusterHistos PRIVATE ${MODULE_NAME})
target_link_libraries(dumpRateHistos PRIVATE Boost::program_options ${MODULE_NAME})

reco_install_targets(${MODULE_NAME} 
                    tpcGUI dumpProjectionHistos
                    dumpClusterHistos dumpRateHistos)
                    
reco_install_root_dict(${MODULE_NAME})

install(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX})
