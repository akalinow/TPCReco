set(MODULE_NAME "Reconstruction")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

<<<<<<< HEAD
###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
###############################################################################
## target definitions #########################################################
###############################################################################
add_library(${MODULE_NAME} SHARED ${sources})
add_library(${TPCRECO_PREFIX}${MODULE_NAME} ALIAS ${MODULE_NAME})
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

target_link_libraries(${MODULE_NAME} ${TPCRECO_LIBRARIES_LOCAL} ${ROOT_LIBRARIES})
# Compiler flags.
#
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/test/> DESTINATION test)
=======
target_link_libraries(
  ${MODULE_NAME}
  PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} DataFormats Utilities
  PRIVATE Resources)

reco_install_targets(${MODULE_NAME})
install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_PREFIX})
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f
