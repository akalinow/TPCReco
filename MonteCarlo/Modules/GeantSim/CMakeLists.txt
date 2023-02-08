SET(src_dir ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB inc "include/*")
FILE(GLOB src "src/*")

SET(geant_sim_sources
        ${src_dir}/GeantSim.cpp
        ${src_dir}/GeantSim.h
        ${inc}
        ${src}
        PARENT_SCOPE
        )



