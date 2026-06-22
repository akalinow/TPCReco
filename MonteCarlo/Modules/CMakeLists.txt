set(MODULE_NAME "MonteCarloModules")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# Ensure ExternalProject uses the NEW behavior for timestamps (policy CMP0135)
cmake_policy(SET CMP0135 NEW)

add_subdirectory(Generator)
add_subdirectory(DummyModule)
add_subdirectory(EventFileExporter)
add_subdirectory(TPCDigitizerRandom)
add_subdirectory(TPCDigitizerSRC)
add_subdirectory(Track3DBuilder)
add_subdirectory(TrackTruncator)
add_subdirectory(TriggerSimulator)
add_subdirectory(ToyIonizationSimulator)

# Function to check internet connectivity
function(check_internet_connectivity URL RESULT_VAR)
  find_program(CURL_EXECUTABLE curl)
  if(CURL_EXECUTABLE)
    execute_process(
      COMMAND ${CURL_EXECUTABLE} -m 5 --connect-timeout 3 -I ${URL}
      OUTPUT_QUIET
      ERROR_QUIET
      RESULT_VARIABLE CURL_RESULT)
    if(CURL_RESULT EQUAL 0)
      set(${RESULT_VAR} TRUE PARENT_SCOPE)
    else()
      set(${RESULT_VAR} FALSE PARENT_SCOPE)
    endif()
  else()
    message(WARNING "curl not found, cannot check internet connectivity")
    set(${RESULT_VAR} TRUE PARENT_SCOPE)
  endif()
endfunction()


#Download strip response files
set(STRIP_RESPONSE_URL "https://mycloud.fuw.edu.pl/index.php/s/x2DBMLei33aKTY3/download")
check_internet_connectivity(${STRIP_RESPONSE_URL} HAVE_INTERNET)
  
if(HAVE_INTERNET)
    include(ExternalProject)
    ExternalProject_Add(
      strip_response_model
      URL ${STRIP_RESPONSE_URL}
      DOWNLOAD_NAME StripResponseModel.tar
      SOURCE_DIR StripResponseModel
      DOWNLOAD_DIR StripResponseModel
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      CONFIGURE_COMMAND "")
  else()
    message(WARNING "No internet access detected. Skipping download of StripResponseModel.")
endif()

if(BUILD_GEANT_MODULE)
  find_package(Geant4 REQUIRED ui_all vis_all)
  find_package(cadmesh REQUIRED) 
  add_subdirectory(GeantSim)
  include_directories(${Geant4_INCLUDE_DIRS})
  include_directories(${CADMESH_INCLUDE_DIRS})
  include_directories(GeantSim/include)

# Download the Geant STL model    
set(GEANT_MODEL_URL "https://mycloud.fuw.edu.pl/index.php/s/znyZHrbPxrmR9Je/download")
check_internet_connectivity(${GEANT_MODEL_URL} HAVE_INTERNET)
if(HAVE_INTERNET)
      include(ExternalProject)
      ExternalProject_Add(
        geant_model
        URL ${GEANT_MODEL_URL}
        DOWNLOAD_NAME elitpc_STL_model_20180302.tar.gz
        SOURCE_DIR Geant_STL_model
        DOWNLOAD_DIR Geant_STL_model
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND "")
    else()
      message(WARNING "No internet access detected. Skipping download of Geant STL model. Setting BUILD_GEANT_MODULE to OFF")
      set(BUILD_GEANT_MODULE OFF)
  endif()
endif()

set(module_sources
    ${generator_sources}
    ${dummy_sources}
    ${event_file_exporter_sources}
    ${geant_sim_sources}
    ${tpc_digitizer_random_sources}
    ${tpc_digitizer_src_sources}
    ${build_track3d_sources}
    ${track_truncator_sources}
    ${trigger_simulator_sources}
    ${toy_ionization_simulator_sources})

reco_add_library(${MODULE_NAME} EXTRA_SOURCES ${module_sources})

if(BUILD_GEANT_MODULE)
  add_dependencies(${MODULE_NAME} geant_model)
  add_dependencies(${MODULE_NAME} strip_response_model)
endif()

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} GenVector Utilities UtilsMC
                        EventGenerator Reconstruction)
if(BUILD_GEANT_MODULE)
  target_link_libraries(${MODULE_NAME} PUBLIC ${Geant4_LIBRARIES}
                                              ${cadmesh_LIBRARIES})
endif()

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})

install(
    TARGETS ${MODULE_NAME}
    EXPORT TPCRecoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
