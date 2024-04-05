set(MODULE_NAME "UtilsMC")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

include_directories(${PROJECT_SOURCE_DIR}/Utilities/include/ ${PROJECT_SOURCE_DIR}/MonteCarlo/UtilsMC)

root_generate_dictionary(G__${TPCRECO_PREFIX}${MODULE_NAME}
                         include/TPCReco/SimEvent.h LINKDEF LinkDef.h)

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)

target_link_libraries(${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} MathMore
                                            GenVector Utilities DataFormats)

reco_install_targets(${MODULE_NAME})

reco_install_root_dict(${MODULE_NAME})

reco_add_test_subdirectory(test)
