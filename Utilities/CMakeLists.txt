set(MODULE_NAME "Utilities")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/CoordinateConverter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/IonRangeCalculator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TPCReco/CommonDefinitions.h
  LINKDEF
  LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)
reco_add_executable(grawls bin/grawls.cpp)

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



target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})
target_link_libraries(
  ${MODULE_NAME}
  PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} Boost::filesystem
         Boost::date_time
  PRIVATE Resources)
target_link_libraries(grawls PRIVATE Boost::program_options ${MODULE_NAME})

target_link_libraries(${MODULE_NAME} PUBLIC ${TPCRECO_LIBRARIES_LOCAL} ${ROOT_LIBRARIES} ${Boost_LIBRARIES} Boost::program_options Boost::filesystem)
target_link_libraries(grawls PRIVATE ${MODULE_NAME} ${TPCRECO_LIBRARIES_LOCAL} ${Boost_LIBRARIES} Boost::program_options)
reco_install_targets(grawls ${MODULE_NAME})
reco_install_root_dict(${MODULE_NAME})
install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)
install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_PREFIX})

reco_add_test_subdirectory(test)
