add_subdirectory(UtilsMC)
add_subdirectory(EventGenerator)
add_subdirectory(examples)
add_subdirectory(Modules)

file(GLOB config_files "${CMAKE_CURRENT_SOURCE_DIR}/config/*.json")

foreach(config_file IN LISTS config_files)
  get_filename_component(config_filename ${config_file} NAME)
  configure_file(
    ${config_file}
    ${CMAKE_CURRENT_BINARY_DIR}/config/${config_filename} @ONLY)
endforeach()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/config
        DESTINATION ${CMAKE_INSTALL_PREFIX})

