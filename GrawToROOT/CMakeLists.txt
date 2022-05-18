set(MODULE_NAME "GrawToROOT")

if(EXISTS ${GET_VERSION_DIR})

  MESSAGE( STATUS "Adding CMake fragment for module:         " ${MODULE_NAME} )
  set(TPCRECO_LIBRARIES ${TPCRECO_LIBRARIES} ${TPCRECO_PREFIX}${MODULE_NAME}  PARENT_SCOPE)
  set(TPCRECO_LIBRARIES_LOCAL ${TPCRECO_LIBRARIES})
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
  include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)

  ###############################################################################
  ## target definitions #########################################################
  ###############################################################################
  add_library(${MODULE_NAME} SHARED ${sources})
  add_library(${TPCRECO_PREFIX}${MODULE_NAME} ALIAS ${MODULE_NAME})
  set_target_properties(${MODULE_NAME} PROPERTIES PREFIX ${TPCRECO_PREFIX})
  
  target_link_libraries(${MODULE_NAME} ${TPCRECO_LIBRARIES_LOCAL} ${ROOT_LIBRARIES} ${GET_LIBRARIES})
  # The data is just added to the executable, because in some IDEs (QtCreator) 
  # files are invisible when they are not explicitly part of the project.
  #
  add_executable(grawToEventTPC bin/grawToEventTPC.cpp)
  add_executable(testEventTPCread test/testEventTPCread.cpp)
  add_executable(testEventTPCreadTChain test/testEventTPCreadTChain.cpp)
  add_executable(grawToEventRaw bin/grawToEventRaw.cpp)
  # Compiler flags.
  #
  MESSAGE( STATUS ${GET_CXX_FLAGS})
  
  target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS} ${GET_CXX_FLAGS})
  target_compile_options(grawToEventTPC PUBLIC ${CMAKE_ROOT_CFLAGS} ${GET_CXX_FLAGS})
  target_compile_options(testEventTPCread PUBLIC ${CMAKE_ROOT_CFLAGS})
  target_compile_options(testEventTPCreadTChain PUBLIC ${CMAKE_ROOT_CFLAGS})
  target_compile_options(grawToEventRaw PUBLIC ${CMAKE_ROOT_CFLAGS} ${GET_CXX_FLAGS})

  # Linker flags
  #
  target_link_libraries(grawToEventTPC PUBLIC ${MODULE_NAME} ${TPCRECO_LIBRARIES} ${GET_LIBRARIES} ${Boost_LIBRARIES})
  target_link_libraries(testEventTPCread PUBLIC ${MODULE_NAME} ${TPCRECO_LIBRARIES})
  target_link_libraries(testEventTPCreadTChain PUBLIC ${MODULE_NAME} ${TPCRECO_LIBRARIES})
  target_link_libraries(grawToEventRaw PUBLIC ${MODULE_NAME} ${TPCRECO_LIBRARIES} ${GET_LIBRARIES} ${Boost_LIBRARIES})

  # All install commands get the same destination. this allows us to use paths
  # relative to the executable.
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib:${GET_VERSION_DIR}/lib)
  MESSAGE( STATUS "CMAKE_INSTALL_RPATH " ${CMAKE_INSTALL_RPATH})

  install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
  install(TARGETS grawToEventTPC RUNTIME DESTINATION bin)
  install(TARGETS testEventTPCread RUNTIME DESTINATION bin)
  install(TARGETS testEventTPCreadTChain RUNTIME DESTINATION bin)
  install(TARGETS grawToEventRaw RUNTIME DESTINATION bin)
  install(DIRECTORY DESTINATION results)
  install(PROGRAMS $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/python/> DESTINATION python)
  
else()
    MESSAGE( WARNING "No GET software found. " ${MODULE_NAME} " will not be build.")

endif()  
