set(MODULE_NAME "GUI")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include/)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/Analysis/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/GUI/include)
root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME} include/TPCReco/MainFrame.h
  include/TPCReco/SelectionBox.h include/TPCReco/DirectoryWatch.h LINKDEF
  LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)
reco_add_executable(tpcGUI bin/tpcGUI.cxx)
reco_add_executable(dumpProjectionHistos bin/dumpProjectionHistos.cpp)
reco_add_executable(dumpClusterHistos bin/dumpClusterHistos.cpp)
reco_add_executable(dumpRateHistos bin/dumpRateHistos.cpp)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

option(DIRECTORYWATCH_ONE_message_DISABLE
       "Define DIRECTORYWATCH_ONE_message_DISABLE variable" OFF)
if(DIRECTORYWATCH_ONE_message_DISABLE)
  message(
    STATUS
      "DirectoryWatch single message disabled (i.e. one message per modified file)"
  )
  target_compile_definitions(${MODULE_NAME} DIRECTORYWATCH_ONE_message_DISABLE)
else()
  message(
    STATUS
      "DirectoryWatch single message enabled (i.e. single message per all modified files)"
  )
endif()

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} DataFormats Reconstruction Analysis
                        Simulation $<$<BOOL:${GET_FOUND}>:GrawToROOT>)
target_link_libraries(tpcGUI PRIVATE Boost::program_options ${MODULE_NAME})
target_link_libraries(dumpProjectionHistos PRIVATE ${MODULE_NAME})
target_link_libraries(dumpClusterHistos PRIVATE ${MODULE_NAME})
target_link_libraries(dumpRateHistos PRIVATE Boost::program_options
                                             ${MODULE_NAME})

reco_install_targets(${MODULE_NAME} tpcGUI dumpProjectionHistos
                     dumpClusterHistos dumpRateHistos)
reco_install_root_dict(${MODULE_NAME})

install(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX})
