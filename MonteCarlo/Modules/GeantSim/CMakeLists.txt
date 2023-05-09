set(src_dir ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB inc "include/*")
file(GLOB src "src/*")

set(geant_sim_sources
    ${src_dir}/GeantSim.cpp ${src_dir}/GeantSim.h ${inc} ${src}
    PARENT_SCOPE)
