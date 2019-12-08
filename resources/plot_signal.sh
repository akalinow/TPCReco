#!/bin/bash

root -l <<EOF 
.include ./include
.L include/MultiKey.h++
.L src/GeometryTPC.cpp+
.L src/EventTPC.cpp++
.L src/UVWprojector.cpp++
.L src/UtilsTPC.cpp+
plot_MCevent("./resources/signal_4He_12C_gamma_8.3MeV__1mm_bins.root","40",nullptr,nullptr,"./results/plot_MCsignal", "./resources/geometry_mini_eTPC_rot90deg.dat",true,true)
EOF
