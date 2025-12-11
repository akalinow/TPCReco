set(MODULE_NAME "EventSources")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/EventSources/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/MonteCarlo/UtilsMC/include)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/MLReconstruction/include)
include_directories(${TF_INCLUDE_DIR})

root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventSourceROOT.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventSourceMC.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventSourceGRAW.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventSourceMultiGRAW.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/EventSourceFactory.h
  LINKDEF
  LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)
reco_add_executable(mcRunController bin/mcRunController.cpp)

# Disable/enable searching next file with N+1 index (default=DISABLED)
option(EVENTSOURCEGRAW_NEXT_FILE_DISABLE
       "Define EVENTSOURCEGRAW_NEXT_FILE_DISABLE variable" ON)
if(EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
  message(STATUS "EventSourceGRAW/EventSourceMultiGRAW automatic next file index disabled")
  target_compile_definitions(${MODULE_NAME}
                             PRIVATE EVENTSOURCEGRAW_NEXT_FILE_DISABLE)
else()
  message(STATUS "EventSourceGRAW/EventSourceMultiGRAW automatic next file index enabled")
endif()

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} 
            DataFormats Reconstruction MLReconstruction Utilities EventGenerator UtilsMC MonteCarloModules
            $<$<BOOL:${GET_FOUND}>:GrawToROOT>)
            
target_link_libraries(mcRunController PRIVATE ${ROOT_LIBRARIES} Utilities ${MODULE_NAME}
                      DataFormats Reconstruction MLReconstruction UtilsMC EventGenerator MonteCarloModules ${Boost_LIBRARIES})

reco_install_targets(${MODULE_NAME} mcRunController)

reco_install_root_dict(${MODULE_NAME})
install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)

reco_add_test_subdirectory(test)
