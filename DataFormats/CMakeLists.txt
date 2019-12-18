set(MODULE_NAME "DataFormats")
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
###############################################################################
## target definitions #########################################################
###############################################################################
# ROOT dictionary for persistens classes
ROOT_GENERATE_DICTIONARY(G__${MODULE_NAME}
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/EventTPC.h
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/GeometryTPC.h 
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/StripTPC.h  
  ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/SigClusterTPC.h  
  LINKDEF ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include/LinkDef.h)

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#
add_library(${MODULE_NAME} SHARED ${sources} G__${MODULE_NAME}.cxx)
target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES})
# Compiler flags.
#
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}_rdict.pcm DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${MODULE_NAME}/lib${MODULE_NAME}.rootmap DESTINATION lib)
