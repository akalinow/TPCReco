set(MODULE_NAME "Utilities")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# ROOT 6.08 dictionary MODULE workaround
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
root_generate_dictionary(
  G__${TPCRECO_PREFIX}${MODULE_NAME} include/TPCReco/CoordinateConverter.h
  include/TPCReco/IonRangeCalculator.h include/TPCReco/CommonDefinitions.h
  LINKDEF LinkDef.h)
#

reco_add_library(${MODULE_NAME} EXTRA_SOURCES
                 G__${TPCRECO_PREFIX}${MODULE_NAME}.cxx)
reco_add_executable(grawls bin/grawls.cpp)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})
target_link_libraries(
  ${MODULE_NAME}
  PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} Boost::filesystem
         Boost::date_time
  PRIVATE Resources)
target_link_libraries(grawls PRIVATE Boost::program_options ${MODULE_NAME})

reco_install_targets(grawls ${MODULE_NAME})
reco_install_root_dict(${MODULE_NAME})
install(PROGRAMS $<1:${CMAKE_CURRENT_SOURCE_DIR}/python/> DESTINATION python)

reco_add_test_subdirectory(test)
