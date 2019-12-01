#pragma once
#ifndef __UTILSTPC_H__
#define __UTILSTPC_H__

#include "TH2D.h"
#include "TH3D.h"
#include "TH2Poly.h"

// helper functions
double my_transfer_function(const double *x, const double *param);   // for TH3D transparency
double my_transfer_function3(const double *x, const double *param);  // for TH3D transparency

void set_custom_rainbow_palette(const int32_t NCont, const float alpha=1.0);
void set_custom_palette(const int32_t index, int32_t NCont=255, const float alpha=1.0);  // backported from ROOT6

TH3D * load_TH3D(std::string input_rootfile, std::string  input_hname);    // load TH3D from ROOT file
bool   compare_TH3D_bins(TH3D *h1, TH3D *h2);                             // check if binnig of two TH3D histograms is exactly the same
TH3D * rescale_TH3D_axes(TH3D *h3_orig, double convert_to_mm_factor=1.0); // creates new TH3D with rescaled XYZ axes of TH3D histogram 
bool   add_to_TH3D(TH3D *h1, TH3D *h2, double weight=1.0);                // performs operation: h1 := h1 + weight*h2 (diff. ranges!)

// helper plot functions
bool plot_TH3D(TH3D *h, std::string canvas_print_fname="");
bool plot_UVW_TH2D(TH2D *h, std::string canvas_print_fname="");
bool plot_UVW_TH2poly(TH2Poly *h, std::string canvas_print_fname="");

void plot_MCevent(std::string input_fname1="resources/bkg.root",  // input ROOT file name for reading
		  std::string input_hname1="Edep-hist",           // input TH3F/TH3D histogram to be plotted
		  std::string input_fname2="resources/sig.root",  // input ROOT file name for reading
		  std::string input_hname2="40",                  // input TH3F/TH3D histogram to be plotted
		  std::string output_prefix="results/plot_MCevent", // prefix for output files with path
		  std::string geom_fname="resources/geometry_mini_eTPC_rot90deg.dat", // GeometryTPC config file
		  bool color_scale_flag=true,                // plot vertical color scale for TH3F/TH3D values
		  bool animation_flag=false,                 // create several PNG files instead of one (for animated GIFs, etc)
		  int rebin=1,                               // rebin factor for X, Y and Z axes
		  const double convert_to_mm_factor = 10.0,   // conversion factor from [input_histogram_units] to [mm]
		  std::string titleX="X [mm]",
		  std::string titleY="Y [mm]",
		  std::string titleZ="Z [mm]",
		  std::string titleVAL="dE/dV [arb.u.]",     // nullptr=no title change
		  double phi0=-50,                           // TView parameter [deg]
		  double theta0=25.0,                        // TView parameter [deg]
		  double color_power=0.8  );

#endif