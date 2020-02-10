#!/bin/bash

root -l <<EOF 
.include ./include
.L src/GeometryTPC.cpp+
.L src/EventTPC.cpp++
plot_MCevent("./resources/bkg_1e7gammas_8.3MeV__1mm_bins.root","Edep-hist", nullptr, nullptr, "./results/plot_MCbkg", "./resources/geometry_mini_eTPC_rot90deg.dat", false, false);
EOF
