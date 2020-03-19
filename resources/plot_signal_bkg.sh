#!/bin/bash

root -l <<EOF 
.include ./include
.L include/MultiKey.h++
.L src/GeometryTPC.cpp+
.L src/EventCharges.cpp++
.L src/UVWprojector.cpp++
.L src/UtilsTPC.cpp+
plot_MCevent("./resources/bkg_1e7gammas_8.3MeV__1mm_bins.root","Edep-hist", "./resources/signal_4He_12C_gamma_8.3MeV__1mm_bins.root","40", "./results/plot_MCevent", "./resources/geometry_mini_eTPC_rot90deg.dat", true, true, 1, 10.0, "X [mm]","Y [mm]","Z [mm]","dE/dV [arb.u.]", -50.0, 25.0, 0.4);
EOF
