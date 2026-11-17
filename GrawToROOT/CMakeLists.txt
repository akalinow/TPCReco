set(MODULE_NAME "GrawToROOT")

# Reading of GRAW files. Self-contained: does not need the GET software.
message(STATUS "Adding CMake fragment for module:\t${MODULE_NAME}")

reco_add_library(${MODULE_NAME})

target_link_libraries(
  ${MODULE_NAME} PUBLIC ${ROOT_LIBRARIES} ${ROOT_EXE_LINKER_FLAGS} DataFormats
                        Utilities)

# WITH_GET marks code paths that read GRAW files. GRAW input is always
# available now, the name is kept for compatibility.
target_compile_definitions(${MODULE_NAME} PUBLIC WITH_GET)

reco_install_targets(${MODULE_NAME})

reco_add_test_subdirectory(test)
