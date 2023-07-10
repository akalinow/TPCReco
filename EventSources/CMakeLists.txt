set(MODULE_NAME "EventSources")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})
reco_add_executable(grawToEventTPC bin/grawToEventTPC.cpp)

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} 
            DataFormats Utilities 
            $<$<BOOL:${GET_FOUND}>:GrawToROOT>)
            
target_link_libraries(grawToEventTPC PRIVATE Boost::program_options ${MODULE_NAME})

reco_install_targets(${MODULE_NAME} grawToEventTPC)

install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)

reco_add_test_subdirectory(test)