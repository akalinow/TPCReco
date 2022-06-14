set(MODULE_NAME "GUI")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )
set(TPCRECO_LIBRARIES_LOCAL ${TPCRECO_LIBRARIES})
set(TPCRECO_LIBRARIES ${TPCRECO_LIBRARIES} ${TPCRECO_PREFIX}${MODULE_NAME}  PARENT_SCOPE)
###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variable
# `sources`
#
file(GLOB_RECURSE sources       ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/*.cpp)

###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/GrawToROOT/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/Analysis/include)

find_package(ROOT REQUIRED COMPONENTS RIO GenVector Gui)
###############################################################################
## target definitions #########################################################
###############################################################################
# ROOT dictionary for persistens classes
ROOT_GENERATE_DICTIONARY(G__${MODULE_NAME}
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/MainFrame.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/SelectionBox.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/DirectoryWatch.h
  LINKDEF ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/RootDictLinkDef.h)

#
add_library(${MODULE_NAME} SHARED ${sources} G__${MODULE_NAME}.cxx)
add_library(${TPCRECO_PREFIX}${MODULE_NAME} ALIAS ${MODULE_NAME})
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

add_executable(tpcGUI bin/tpcGUI.cxx)
add_executable(dumpProjectionHistos bin/dumpProjectionHistos.cpp)
add_executable(dumpClusterHistos bin/dumpClusterHistos.cpp)
add_executable(dumpRateHistos bin/dumpRateHistos.cpp)
# Compiler flags.
#
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(tpcGUI PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(${MODULE_NAME} ${TPCRECO_LIBRARIES_LOCAL} ${GRAW_TO_ROOT_LIBRARY} ${ROOT_LIBRARIES})
target_link_libraries(tpcGUI PUBLIC ${TPCRECO_LIBRARIES} ${Boost_LIBRARIES} ${GRAW_TO_ROOT_LIBRARY} ${MODULE_NAME})
target_link_libraries(dumpProjectionHistos PUBLIC ${TPCRECO_LIBRARIES} ${GRAW_TO_ROOT_LIBRARY} ${MODULE_NAME})
target_link_libraries(dumpClusterHistos PUBLIC ${TPCRECO_LIBRARIES} ${GRAW_TO_ROOT_LIBRARY} ${MODULE_NAME})
target_link_libraries(dumpRateHistos PUBLIC ${TPCRECO_LIBRARIES} ${GRAW_TO_ROOT_LIBRARY} ${MODULE_NAME})

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(TARGETS tpcGUI RUNTIME DESTINATION bin)
install(TARGETS dumpProjectionHistos RUNTIME DESTINATION bin)
install(TARGETS dumpClusterHistos RUNTIME DESTINATION bin) 
install(TARGETS dumpRateHistos RUNTIME DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}_rdict.pcm DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}.rootmap DESTINATION lib)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/config/> DESTINATION config)


