set(MODULE_NAME "Analysis")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/Analysis/include/)
root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/HIGS_trees_dataFormat.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/RawSignal_tree_dataFormat.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/TrackDiffusion_tree_dataFormat.h
  LINKDEF
  LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

add_executable(makeTrackTree bin/makeTrackTree.cpp)
add_executable(makeMCTrackTree bin/makeMCTrackTree.cpp)
add_executable(recoEventsAnalysis bin/recoEventsAnalysis.cpp)
add_executable(recoEventsComparison bin/recoEventsComparison.cpp)
add_executable(rawSignalAnalysis bin/rawSignalAnalysis.cpp)
add_executable(rawTrackDiffusionAnalysis bin/rawTrackDiffusionAnalysis.cpp)
add_executable(recoEventsClean bin/recoEventsClean.cpp)
add_executable(recoEventsDiff bin/recoEventsDiff.cpp)
add_executable(recoEnergyScaleFitter bin/recoEnergyScaleFitter.cpp)

target_link_libraries(
  ${MODULE_NAME} PUBLIC Utilities DataFormats Reconstruction EventSources GrawToROOT ${ROOT_LIBRARIES}
                        ${ROOT_EXE_LINKER_FLAGS})

target_link_libraries(makeTrackTree PRIVATE ${MODULE_NAME}
                                            Boost::program_options GUI)
target_link_libraries(makeMCTrackTree PRIVATE ${MODULE_NAME}
                                              Boost::program_options)
target_link_libraries(recoEventsAnalysis PRIVATE ${MODULE_NAME}
                                                 Boost::program_options)
target_link_libraries(recoEventsComparison PRIVATE ${MODULE_NAME}
                                                   Boost::program_options)
target_link_libraries(rawSignalAnalysis PRIVATE ${MODULE_NAME}
                                                Boost::program_options)
target_link_libraries(rawTrackDiffusionAnalysis PRIVATE ${MODULE_NAME}
                                                Boost::program_options)
target_link_libraries(recoEventsClean PRIVATE ${MODULE_NAME}
                                              Boost::program_options)
target_link_libraries(recoEventsDiff PRIVATE ${MODULE_NAME}
                                             Boost::program_options)
target_link_libraries(recoEnergyScaleFitter PRIVATE ${MODULE_NAME}
					            Boost::program_options)

reco_install_targets(
  ${MODULE_NAME}
  makeTrackTree
  makeMCTrackTree
  recoEventsAnalysis
  recoEventsComparison
  rawSignalAnalysis
  rawTrackDiffusionAnalysis
  recoEventsClean
  recoEventsDiff
  recoEnergyScaleFitter)
reco_install_root_dict(${MODULE_NAME})
install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)
install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_PREFIX})

reco_add_test_subdirectory(test)
