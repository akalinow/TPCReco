add_unit_test(dEdxFitter_tst Reconstruction)
target_compile_definitions(dEdxFitter_tst PRIVATE RESOURCES_DIR="${PROJECT_SOURCE_DIR}/resources/")
add_unit_test(StripResponseCalculator_tst Reconstruction)
target_compile_definitions(StripResponseCalculator_tst PRIVATE RESOURCES_DIR="${PROJECT_SOURCE_DIR}/resources/")
