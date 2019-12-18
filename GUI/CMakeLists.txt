set(MODULE_NAME "GUI")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )

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

find_package(ROOT REQUIRED COMPONENTS RIO GenVector Gui)
###############################################################################
## target definitions #########################################################
###############################################################################
# ROOT dictionary for persistens classes
ROOT_GENERATE_DICTIONARY(G__${MODULE_NAME} ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/MainFrame.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/ScrollFrame.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/SelectionBox.h
  LINKDEF ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/RootDictLinkDef.h)

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#
add_library(${MODULE_NAME} SHARED ${sources} G__${MODULE_NAME}.cxx)

add_executable(tpcGUI bin/tpcGUI.cxx)

# Compiler flags.
#
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})
target_compile_options(tpcGUI PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(${MODULE_NAME} DataFormats Reconstruction ${ROOT_LIBRARIES})
target_link_libraries(tpcGUI PUBLIC DataFormats Reconstruction ${MODULE_NAME} -lSpectrum)

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(TARGETS tpcGUI RUNTIME DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}_rdict.pcm DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}.rootmap DESTINATION lib)
install(FILES $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/config/> DESTINATION config)


