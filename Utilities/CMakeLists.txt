set(MODULE_NAME "Utilities")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )
set(TPCRECO_LIBRARIES_LOCAL ${TPCRECO_LIBRARIES})
set(TPCRECO_LIBRARIES ${TPCRECO_LIBRARIES} ${TPCRECO_PREFIX}${MODULE_NAME} PARENT_SCOPE)

###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variable
# `sources`
#
find_package(Boost COMPONENTS filesystem REQUIRED)
file(GLOB_RECURSE sources       ${PROJECT_SOURCE_DIR}/${MODULE_NAME}/src/*.cpp)

###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${PROJECT_SOURCE_DIR}/${MODULE_NAME}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
###############################################################################
## target definitions #########################################################
###############################################################################
# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
#
add_executable(grawls bin/grawls.cpp)
add_library(${MODULE_NAME} SHARED ${sources})
add_library(${TPCRECO_PREFIX}${MODULE_NAME} ALIAS ${MODULE_NAME})
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

# Compiler flags.
#
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

target_link_libraries(${MODULE_NAME} PUBLIC ${TPCRECO_LIBRARIES_LOCAL} ${ROOT_LIBRARIES} Boost::filesystem Boost::date_time)
target_link_libraries(grawls PRIVATE ${MODULE_NAME} ${TPCRECO_LIBRARIES_LOCAL} Boost::program_options)

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS grawls RUNTIME DESTINATION bin)
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
install(PROGRAMS $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/python/> DESTINATION python)
file(GLOB ROOT_CXX_MACROS "${PROJECT_SOURCE_DIR}/${MODULE_NAME}/test/*.cxx")
install(FILES $<1:${ROOT_CXX_MACROS}> DESTINATION test)

reco_add_test_subdirector(test)
