set(MODULE_NAME "WebServerClient")
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

# Web based GUI (tpcWebGUI): Crow web framework on the server side,
# Plotly.js in the browser.

include(FetchContent)

# Crow: header-only C++ HTTP/WebSocket framework, used with Boost.Asio.
# Only the headers are needed: SOURCE_SUBDIR points to a directory without
# CMakeLists.txt, so Crow's own CMake project is not added to the build.
set(CROW_VERSION 1.3.4)
FetchContent_Declare(
  crow
  URL https://github.com/CrowCpp/Crow/archive/refs/tags/v${CROW_VERSION}.tar.gz
  URL_HASH SHA256=6be759b3a648b41f0adc3a8408a53fd0243775217b4e32e8101d69e4aadf48cc
  SOURCE_SUBDIR headers-only)
FetchContent_MakeAvailable(crow)

find_package(Threads REQUIRED)
add_library(crow_headers INTERFACE)
# SYSTEM: no -Wall -Werror diagnostics from the Crow headers
target_include_directories(crow_headers SYSTEM INTERFACE ${crow_SOURCE_DIR}/include)
target_compile_definitions(crow_headers INTERFACE CROW_USE_BOOST)
target_link_libraries(crow_headers INTERFACE Boost::headers Threads::Threads)

# Plotly.js for the browser: taken from the python plotly package when available
# (no network needed), otherwise downloaded.
set(PLOTLY_JS_VERSION 3.1.1)
set(PLOTLY_JS ${CMAKE_CURRENT_BINARY_DIR}/web/plotly.min.js)
if(NOT EXISTS ${PLOTLY_JS})
  execute_process(
    COMMAND python3 -c "import os, plotly; print(os.path.join(os.path.dirname(plotly.__file__), 'package_data', 'plotly.min.js'))"
    OUTPUT_VARIABLE PLOTLY_JS_PYTHON
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(PLOTLY_JS_PYTHON AND EXISTS ${PLOTLY_JS_PYTHON})
    message(STATUS "Using Plotly.js from ${PLOTLY_JS_PYTHON}")
    configure_file(${PLOTLY_JS_PYTHON} ${PLOTLY_JS} COPYONLY)
  else()
    message(STATUS "Downloading Plotly.js ${PLOTLY_JS_VERSION}")
    file(DOWNLOAD https://cdn.jsdelivr.net/npm/plotly.js-dist-min@${PLOTLY_JS_VERSION}/plotly.min.js
         ${PLOTLY_JS} STATUS PLOTLY_DOWNLOAD_STATUS)
    list(GET PLOTLY_DOWNLOAD_STATUS 0 PLOTLY_DOWNLOAD_CODE)
    if(NOT PLOTLY_DOWNLOAD_CODE EQUAL 0)
      message(WARNING "Plotly.js not available: tpcWebGUI will not display plots.")
    endif()
  endif()
endif()

include_directories(${PROJECT_SOURCE_DIR}/DataFormats/include/)
include_directories(${PROJECT_SOURCE_DIR}/Reconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/MLReconstruction/include)
include_directories(${PROJECT_SOURCE_DIR}/GrawToROOT/include)
include_directories(${TF_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/Utilities/include)
include_directories(${PROJECT_SOURCE_DIR}/FileOutput/include)

reco_add_library(${MODULE_NAME})
reco_add_executable(tpcWebGUI bin/tpcWebGUI.cxx)

# Crow and std::filesystem/std::optional need C++17
set_target_properties(${MODULE_NAME} tpcWebGUI PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_compile_options(${MODULE_NAME} PUBLIC ${CMAKE_ROOT_CFLAGS})
target_link_libraries(
  ${MODULE_NAME}
  PUBLIC ${ROOT_LIBRARIES} DataFormats EventSources Reconstruction FileOutput GrawToROOT Utilities
  PRIVATE crow_headers)
target_link_libraries(tpcWebGUI PRIVATE ${MODULE_NAME} crow_headers)

reco_install_targets(${MODULE_NAME} tpcWebGUI)
install(DIRECTORY web/ DESTINATION ${CMAKE_INSTALL_PREFIX}/web)
install(FILES ${PLOTLY_JS} DESTINATION ${CMAKE_INSTALL_PREFIX}/web OPTIONAL)
