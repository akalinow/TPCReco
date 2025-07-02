set(MODULE_NAME "FileOutput")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

add_executable(tpcWriter bin/tpcWriter.cpp)

target_link_libraries(${MODULE_NAME} PUBLIC Utilities DataFormats 
                     Reconstruction EventSources ${ROOT_LIBRARIES}
                     ${ROOT_EXE_LINKER_FLAGS})

target_link_libraries(tpcWriter PRIVATE ${MODULE_NAME} Boost::program_options)
					  
reco_install_targets(${MODULE_NAME} tpcWriter)
#install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)
install(DIRECTORY config DESTINATION ${CMAKE_INSTALL_PREFIX})

reco_add_test_subdirectory(test)

