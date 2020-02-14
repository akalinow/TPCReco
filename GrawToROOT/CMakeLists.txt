set(MODULE_NAME "GrawToROOT")

if(EXISTS ${GET_VERSION_DIR})

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
  include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)

  ###############################################################################
  ## target definitions #########################################################
  ###############################################################################
  add_library(${MODULE_NAME} SHARED ${sources})
  target_link_libraries(${MODULE_NAME} ${ROOT_LIBRARIES} ${GET_LIBRARIES} DataFormats)
  # The data is just added to the executable, because in some IDEs (QtCreator) 
  # files are invisible when they are not explicitly part of the project.
  #
  add_executable(grawToEventTPC bin/grawToEventTPC.cpp)
  add_executable(testEventTPCread test/testEventTPCread.cpp)
  add_executable(testEventTPCreadTChain test/testEventTPCreadTChain.cpp)
  # Compiler flags.
  #
  target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS} ${GET_CXX_FLAGS})
  target_compile_options(grawToEventTPC PUBLIC ${CMAKE_ROOT_CFLAGS} ${GET_CXX_FLAGS})
  target_compile_options(testEventTPCread PUBLIC ${CMAKE_ROOT_CFLAGS})
  target_compile_options(testEventTPCreadTChain PUBLIC ${CMAKE_ROOT_CFLAGS})

  # Linker flags
  #
  target_link_libraries(grawToEventTPC PUBLIC DataFormats Utilities GrawToROOT ${GET_LIBRARIES})
  target_link_libraries(testEventTPCread PUBLIC DataFormats)
  target_link_libraries(testEventTPCreadTChain PUBLIC DataFormats)

  # All install commands get the same destination. this allows us to use paths
  # relative to the executable.
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib:${GET_VERSION_DIR}/lib)
  MESSAGE( STATUS "CMAKE_INSTALL_RPATH " ${CMAKE_INSTALL_RPATH})

  install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION lib)
  install(TARGETS grawToEventTPC RUNTIME DESTINATION bin)
  install(TARGETS testEventTPCread RUNTIME DESTINATION bin)
  install(TARGETS testEventTPCreadTChain RUNTIME DESTINATION bin)
  install(DIRECTORY DESTINATION results)
  install(PROGRAMS $<1:${PROJECT_SOURCE_DIR}/${MODULE_NAME}/python/> DESTINATION python)
  
  else()
    MESSAGE( WARNING "No GET software found. " ${MODULE_NAME} " will not be build.")

endif()  
