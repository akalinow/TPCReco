set(MODULE_NAME "UtilsMC")
MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )
set(TPCRECO_LIBRARIES ${TPCRECO_LIBRARIES} " " ${TPCRECO_PREFIX}${MODULE_NAME}  PARENT_SCOPE)
set(TPCRECO_LIBRARIES ${TPCRECO_LIBRARIES} ${TPCRECO_PREFIX}${MODULE_NAME} PARENT_SCOPE)
###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variable
# `sources`
#
file(GLOB_RECURSE sources       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

ADD_SUBDIRECTORY(Modules)

SET(util_sources
        ${sources}
        #${module_sources}
        )

###############################################################################
## dependencies ###############################################################
###############################################################################
include_directories(${ROOTINCDIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include)
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
###############################################################################
## target definitions #########################################################
###############################################################################
add_library(${MODULE_NAME} SHARED ${util_sources} UtilsMcDict.cxx)
add_library(${TPCRECO_PREFIX}${MODULE_NAME} ALIAS ${MODULE_NAME})
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})

target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} MathMore GenVector Utilities)
# Compiler flags.
target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

ROOT_GENERATE_DICTIONARY(UtilsMcDict
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SimEvent.h
        LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h
        MODULE ${MODULE_NAME})

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)

install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libUtilsMC_rdict.pcm
        ${CMAKE_CURRENT_BINARY_DIR}/libUtilsMC.rootmap
        DESTINATION lib)

reco_add_test_subdirector(test)


