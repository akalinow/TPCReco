set(MODULE_NAME "GrawToROOT")

##Check GET software dependencies first
set(GET_VERSION_DIR $ENV{GET_DIR}/GetSoftware_$ENV{GET_RELEASE})
include_directories(${GET_VERSION_DIR}/include)
link_directories(${GET_VERSION_DIR}/lib)
MESSAGE( STATUS "GET_VERSION_DIR:            " ${GET_VERSION_DIR})
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
  # The data is just added to the executable, because in some IDEs (QtCreator) 
  # files are invisible when they are not explicitly part of the project.
  #
  add_executable(grawToROOT test/test1.cpp)
  # Compiler flags.
  #
  target_compile_options(grawToROOT PUBLIC ${CMAKE_ROOT_CFLAGS} -Wno-deprecated-declarations)

  # Linker flags
  #
  target_link_libraries(grawToROOT PUBLIC DataFormats Utilities libget-root.so libcobo-frame-graw2frame.so libMultiFrame.so)

  # All install commands get the same destination. this allows us to use paths
  # relative to the executable.
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib ${GET_VERSION_DIR}/lib)

  install(TARGETS grawToROOT RUNTIME DESTINATION bin)
  install(DIRECTORY DESTINATION results)

  else()
    MESSAGE( WARNING "No GET software found. " ${MODULE_NAME} " will not be build.")

endif()  
