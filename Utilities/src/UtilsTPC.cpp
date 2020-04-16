#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TH2F.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TPaletteAxis.h"
#include "TList.h"
#include "TF1.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TColor.h"

#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "UtilsTPC.h"

// helper plot functions
bool plot_TH3D(TH3D* h, const char* canvas_print_fname) { return true; }
bool plot_UVW_TH2D(TH2D* h, const char* canvas_print_fname) { return true; }
bool plot_UVW_TH2poly(TH2Poly* h, const char* canvas_print_fname) { return true; }

void plot_MCevent(const char* input_fname1,  // input ROOT file name for reading (required)
	const char* input_hname1,  // input TH3F/TH3D histogram to be plotted (required)
	const char* input_fname2,  // input ROOT file name for reading (optional, NULL=none)
	const char* input_hname2,  // input TH3F/TH3D histogram to be plotted (optional, NULL=none)
	const char* output_fname,  // prefix for output files with path (optional)
	std::string geom_fname,    // GeometryTPC config file (required)
	bool color_scale_flag,     // plot vertical color scale for TH3F/TH3D values (default=false)
	bool animation_flag,       // create several PNG files instead of one (for animated GIFs, default=false)
	int rebin,                 // rebin factor for X, Y and Z axes (default=none=1)
	const double convert_to_mm_factor, // conversion factor from [input_histogram_units] to [mm]
	const char* titleX,
	const char* titleY,
	const char* titleZ,
	const char* titleVAL,     // NULL=no title change
	double phi0,              // TView parameter [deg]
	double theta0,            // TView parameter [deg]
	double color_power) {     // color TransferFunction power parameter: val^power, where power>0, 0<val<1 

	const bool zoom_10cm = true;
	//  const int palette = 90; // Neon
	// const int palette = 55; // Rainbow
	// const int palette = 109; // Cool
	const int palette = 56; // Inverted Dark Body Radiator

#if defined (__CINT__) || defined(__MAKECINT__)
	std::cout << "Calling gROOT->Reset" << std::endl << std::flush;
	gROOT->Reset();
#endif

	gStyle->SetOptStat(0);

	std::string prefix(output_fname);

	// READ SIMULATED 3D ENERGY DEPOSITS FROM ROOT FILE(S)
	// ___________________________________________________

	// 1st histogram (mandatory)
	TH3D* h3_orig = load_TH3D(input_fname1, input_hname1);
	if (!h3_orig) return; // file1 is required, file2 is optional

	// 2nd histogram (optional)
	TH3D* h3_orig2 = load_TH3D(input_fname2, input_hname2);

	// merging BACKGROUND and SIGNAL (optional)
	if (h3_orig2 && !add_to_TH3D(h3_orig, h3_orig2)) {
		std::cerr << "ERROR: Failed to merge two TH3D histograms!" << std::endl;
		return;
	}

	// rescaling XYZ axes (optional)
	TH3D* h3_scaled = rescale_TH3D_axes(h3_orig, convert_to_mm_factor);

	// rebinnig XYZ axes (optional)
	TH3D* h3 = (TH3D*)(rebin > 1 ?
		h3_scaled->Rebin3D(rebin, rebin, rebin, h3_orig->GetName()) :
		h3_scaled);
	if (!h3) return;
	std::cout << "TH3D after rebin:  MIN = " << h3->GetMinimum() << std::endl;
	std::cout << "TH3D after rebin:  MAX = " << h3->GetMaximum() << std::endl;
	h3_orig->SetName(Form("%s_orig", h3_orig->GetName())); // rename to *_orig

	// changing XY plot range (optional)
	if (zoom_10cm) {
		h3->GetXaxis()->SetRangeUser(-50.0, 50.0);   // ZOOM-IN X-axis
		h3->GetYaxis()->SetRangeUser(-50.0, 50.0);   // ZOOM-IN Y-axis
	}


	// PLOT TH3D INPUT
	// _______________

	//  set_custom_rainbow_palette(255);
	set_custom_palette(palette);
	gStyle->SetCanvasPreferGL(true);

	TCanvas* c0 = new TCanvas("c0", "", 2, 78, 1200, 800);
	c0->Range(-1, -1, 1, 1);
	c0->SetFillColor(0);
	c0->SetBorderMode(0);
	c0->SetBorderSize(0);
	c0->SetFrameLineColor(0);
	c0->SetFrameBorderMode(0);
	gStyle->SetOptStat(0);
	gStyle->SetOptTitle(0);

	//
	// On request plots a nice vertical color scale for TH3F/TH3D values. 
	// Standard "GLCOL Z" option leads to ugly results.
	//
	if (color_scale_flag) {
		c0->Divide(2, 1);

		c0->cd(2);
		gPad->SetPad(0.0, 0.0, 0.82, 1.0);  // this TPad is used do draw the actual 3D plot which takes 85% of TCanvas width

		c0->cd(1);
		gPad->SetPad(0.0, 0.0, 1.0, 1.0);   // this TPad is used only to draw a nice TPaletteAxis for Z axis

		// create dummy (non-empty) TH2F in order to draw nice coloured Z-scale on the right hand side of TCanvas
		TH2F* h = new TH2F("h", "", 1, 0., 1., 1, 0., 1.); h->Fill(-1, -1);
		h->SetMinimum(h3->GetMinimum());
		h->SetMaximum(h3->GetMaximum());
		h->GetZaxis()->SetTitle(titleVAL);
		h->GetZaxis()->CenterTitle(true);
		h->GetZaxis()->SetTitleOffset(1.6);
		h->GetZaxis()->SetLabelOffset(0.01);
		//    h->GetZaxis()->SetRangeUser(h3->GetMinimum(), h3->GetMaximum()); 
		h->GetZaxis()->SetRangeUser(0., h3->GetMaximum());
		h->SetFillColor(0);
		h->SetLineColor(0);
		h->Draw("COLZ A");

		gPad->SetRightMargin(0.3); // to suppress dummy TH2F
		gPad->SetLeftMargin(0.3); // to suppress dummy TH2F
		gPad->SetTopMargin(0.3); // to suppress dummy TH2F
		gPad->SetBottomMargin(0.3); // to suppress dummy TH2F

		gPad->Update();  // needed for accesing TFrame, TPaletteAxis
		TPaletteAxis* tpal = (TPaletteAxis*)h->GetListOfFunctions()->FindObject("palette");
		if (tpal) {
			tpal->SetX1NDC(0.82);   // Z-scale will take 5% of TCanvas width
			tpal->SetX2NDC(0.87);
			tpal->SetY1NDC(0.1);   // Z-scale will take 5% of TCanvas width
			tpal->SetY2NDC(0.9);
			tpal->SetLineWidth(2);
		}
		gPad->Update();
		gPad->Modified();

		c0->cd(2);

	} // end of if(color_scale_flag...

	//
	// Plot the actual TH3F/TH3D
	//
	gPad->SetPhi(phi0);
	gPad->SetTheta(theta0);

	//
	// Associate color transfer functions with TH3F histograms
	//
	TList* lf = h3->GetListOfFunctions();
	std::unique_ptr<TF1> tf;
	if (lf != nullptr) {
		lf->Clear(); // remove all previous functions just in case
		// function with 2 parameters (range of hist values)
		tf = std::make_unique<TF1>("TransferFunction", my_transfer_function3, h3->GetMinimum(), h3->GetMaximum(), 3);
		tf->SetParameters(h3->GetMinimum(), h3->GetMaximum(), color_power);
		lf->Add(tf.get());
	}

	if (titleX) h3->GetXaxis()->SetTitle(titleX);
	h3->GetXaxis()->CenterTitle(true);
	h3->GetXaxis()->SetTitleOffset(2);
	if (titleY) h3->GetYaxis()->SetTitle(titleY);
	h3->GetYaxis()->CenterTitle(true);
	h3->GetYaxis()->SetTitleOffset(2);
	if (titleZ) h3->GetZaxis()->SetTitle(titleZ);
	h3->GetZaxis()->CenterTitle(true);
	h3->GetZaxis()->SetTitleOffset(2);
	//  h3->GetXaxis()->SetTicks("-");
	//  h3->GetYaxis()->SetTicks("-");
	//  h3->GetZaxis()->SetTicks("-");
	h3->GetXaxis()->SetNdivisions(510);
	h3->GetYaxis()->SetNdivisions(510);
	h3->GetZaxis()->SetNdivisions(510);

	//
	// Here are different 3D histo drawing options:
	//
	const char* option = "GLCOL";   // color according to my_transfer_function(<bin content>)
	//  const char *option="GLCOLZ";  // color according to my_transfer_function(<bin content>) + color Z scale (quite ugly)
	//  const char *option="GLBOX";   // box size prop. to <bin contetnt>
	//  const char *option="GLBOX1";  // sphere size prop. to <bin content>
	//  h3->SetFillColor(kRed);  // valid for GLBOX, GLBOX1 only

	option = "box";
	h3->Draw(option);
	//  h3->DrawCopy(option);

	gPad->Update();
	gPad->Modified();
	c0->Modified();
	c0->Update();
	c0->SetSelected(c0);

	//
	// Produces single and/or multiple PNG output files:
	//
	const std::string suffix((zoom_10cm ? "__zoom10cm" : ""));

	// Plots a single 3D view using (phi0, theta0) 

	// Here are different options of saving the output plot:

	// PNG - works fine (transparency is preserved)
	c0->Print(Form("%s__input_TH3D%s.png", prefix.c_str(), suffix.c_str()), "png");

	// C - works fine (transparency & colors are preserved)
	//    c0->Print(Form("%s__input_TH3D%s.C",prefix.c_str(),suffix.c_str()), "cxx");

	// PS, EPS - produce opaque boxes (transparency is broken!) 
	//    c0->Print(Form("%s__input_TH3D%s.eps",prefix.c_str(),suffix.c_str()), "eps");

	// ROOT - to read stored canvas with GL object use:
	// root -l YOUR_FILE.root
	// _file0->cd()
	// gStyle->SetCanvasPreferGL(kTRUE);
	// gStyle->SetPalette(1);
	// c0->Draw();
	c0->Print(Form("%s__input_TH3D%s.root", prefix.c_str(), suffix.c_str()));


	return;

	if (animation_flag) { // creates multiple PNG files for different viewing angles

		const int Nframes = 15;
		const double DeltaPhi = 60.; // deg
		//    const DeltaColor=0; // transparency parameter

		for (int i = 0; i < Nframes; i++) {

			//
			// On request plots a nice vertical color scale for TH3F/TH3D values. 
			// Standard "GLCOL Z" option leads to ugly results.
			//
			if (color_scale_flag) {

				c0->Clear();
				c0->Divide(2, 1);

				c0->cd(2);
				gPad->SetPad(0.0, 0.0, 0.82, 1.0);  // this TPad is used do draw the actual 3D plot which takes 85% of TCanvas width

				c0->cd(1);
				gPad->SetPad(0.0, 0.0, 1.0, 1.0);   // this TPad is used only to draw a nice TPaletteAxis for Z axis

				// create dummy (non-empty) TH2F in order to draw nice coloured Z-scale on the right hand side of TCanvas
				auto h = std::make_unique<TH2F>("h", "", 1, 0., 1., 1, 0., 1.);
				h->Fill(-1, -1);
				h->SetMinimum(h3->GetMinimum());
				h->SetMaximum(h3->GetMaximum());
				h->GetZaxis()->SetTitle(titleVAL);
				h->GetZaxis()->CenterTitle(true);
				h->GetZaxis()->SetTitleOffset(1.6);
				h->GetZaxis()->SetLabelOffset(0.01);
				//    h->GetZaxis()->SetRangeUser(h3->GetMinimum(), h3->GetMaximum()); 
				h->GetZaxis()->SetRangeUser(0., h3->GetMaximum());
				h->SetFillColor(0);
				h->SetLineColor(0);
				h->Draw("COLZ A");

				gPad->SetRightMargin(0.3); // to suppress dummy TH2F
				gPad->SetLeftMargin(0.3); // to suppress dummy TH2F
				gPad->SetTopMargin(0.3); // to suppress dummy TH2F
				gPad->SetBottomMargin(0.3); // to suppress dummy TH2F

				gPad->Update();  // needed for accesing TFrame, TPaletteAxis
				auto tpal = (TPaletteAxis*)h->GetListOfFunctions()->FindObject("palette");
				if (tpal) {
					tpal->SetX1NDC(0.82);   // Z-scale will take 5% of TCanvas width
					tpal->SetX2NDC(0.87);
					tpal->SetY1NDC(0.1);   // Z-scale will take 5% of TCanvas width
					tpal->SetY2NDC(0.9);
					tpal->SetLineWidth(2);
				}
				gPad->Update();
				gPad->Modified();

				c0->cd(2);

			} // end of if(color_scale_flag...


			gPad->SetPhi(phi0 + DeltaPhi * (-0.5 + (1.0 * i) / (Nframes - 1)));
			gPad->SetTheta(theta0);
			gPad->RedrawAxis();

			//      tf->SetParameter(2, (dir+1)*0.1 );
			h3->DrawCopy(option);

			gPad->Modified();
			gPad->Update();

			c0->Modified();
			c0->Update();

			c0->SetSelected(c0);
			c0->Print(Form("%s__input_TH3D%s__frame%d.png", prefix.c_str(), suffix.c_str(), i), "png");
		}
	} // end of if(!animation_flag...


	// INITIALIZE UVW-projector
	// ________________________

	Geometry(geom_fname);
	auto p = std::make_unique<UVWprojector>(100, 25, 25); //  100, 100, 100);
	p->SetEvent3D(*h3);
	std::cout << p->GetAreaNpoints() << std::endl;
	std::cout << Geometry().GetTH2PolyPartitionX() << std::endl;
	std::cout << Geometry().GetTH2PolyPartitionY() << std::endl;
	std::cout << "integral=" << p->GetEventIntegral() << std::endl;
	if (!p) return;


	// MAKE UVW PROJECTIONS
	// ____________________
	gStyle->SetCanvasPreferGL(false);
	gStyle->SetOptTitle(false);
	set_custom_palette(palette);

	auto c1 = std::make_unique<TCanvas>("c1", "c1", 900, 800);
	c1->cd();
	//  TH2Poly *tp1 = (TH2Poly*) p->DrawTH2Poly("COL2Z");
	auto tp1 = p->GetStripProfile_TH2Poly();
	if (!tp1) {
		std::cerr << "ERROR: Strip profile TH2Poly is NULL !!!" << std::endl;
		return;
	}
	tp1->Draw("COLZ");
	std::cout << "integral=" << tp1->Integral() << std::endl;

	tp1->SetTitle("Time-integrated charge per strip");
	if (titleX) tp1->GetXaxis()->SetTitle(titleX);
	tp1->GetXaxis()->CenterTitle(true);
	tp1->GetXaxis()->SetTitleOffset(1.4);
	if (titleY) tp1->GetYaxis()->SetTitle(titleY);
	tp1->GetYaxis()->CenterTitle(true);
	tp1->GetYaxis()->SetTitleOffset(1.4);
	tp1->GetZaxis()->SetTitle("Charge [arb.u.]");
	tp1->GetZaxis()->CenterTitle(true);
	tp1->GetZaxis()->SetTitleOffset(1.4);

	gPad->Update(); // updates list of functions to get TPaletteAxis
	tp1->GetListOfFunctions()->ls();
	TPaletteAxis* pa1 = (TPaletteAxis*)tp1->GetListOfFunctions()->FindObject("palette");
	pa1->SetX2NDC(0.9);
	pa1->SetX1NDC(0.85);
	pa1->SetY2NDC(0.9);
	pa1->SetY1NDC(0.1);

	//  tp1->GetXaxis()->SetNdivisions(510);
	//  tp1->GetYaxis()->SetNdivisions(510); 

	gPad->SetRightMargin(0.2);
	gPad->SetLeftMargin(0.1);
	gPad->SetTopMargin(0.1);
	gPad->SetBottomMargin(0.1);
	//  gPad->ReDrawAxis();
	gPad->Update();
	gPad->Modified();
	c1->Modified();
	c1->Update();
	c1->SetSelected(c1.get());

	// PNG - works fine (transparency is preserved)
	c1->Print(Form("%s__output_TH2poly.png", prefix.c_str()), "png");

	// C - works fine (transparency & colors are preserved)
	//  c1->Print(Form("%s__output_TH2poly.C",prefix.c_str()), "cxx");

	// PS, EPS - produce opaque boxes (transparency is broken!) 
	//  c1->Print(Form("%s__output_TH2poly.eps",prefix.c_str()), "eps");

	// ROOT - to read stored canvas with GL object use:
	// root -l YOUR_FILE.root
	// _file0->cd()
	// gStyle->SetPalette(1);
	// c1->Draw();
	c1->Print(Form("%s__output_TH2poly.root", prefix.c_str()));

	double zmax = 0.0;
	double zmin = 0.0;

	gStyle->SetCanvasPreferGL(false);
	gStyle->SetOptTitle(false);
	set_custom_palette(palette);
	auto c2 = std::make_unique<TCanvas>("c2", "c2", 900, (int)(900 * 3.0));
	c2->Divide(1, 3);
	c2->cd();
	std::map<direction, std::shared_ptr<TH2D>> ts2;

	zmin = 0.0;
	zmax = 0.0;

	for (auto dir : dirs) {
		c2->cd(int(dir) + 1);
		//    ts2[dir] = (TH2D*) p->DrawStripVsTime(dir, "COL2Z");
		ts2[dir] = p->GetStripVsTime_TH2D(dir);
		if (!ts2[dir]) {
			std::cerr << "ERROR: Strip vs time TH2D[" << dir << "] is NULL !!!" << std::endl;
			return;
		}
		ts2[dir]->Draw("COL2Z");
		std::cout << "dir=" << dir << ": ptr=" << ts2[dir] << ", integral=" << ts2[dir]->Integral() << std::endl;
		if (ts2[dir]->GetMaximum() > zmax) zmax = ts2[dir]->GetMaximum();
		if (ts2[dir]->GetMinimum() < zmin) zmin = ts2[dir]->GetMinimum();
	}
	for (auto dir : dirs) {
		c2->cd(int(dir) + 1);

		ts2[dir]->SetMinimum(zmin);
		ts2[dir]->SetMaximum(zmax);

		ts2[dir]->SetTitle("Charge(time) per strip");
		ts2[dir]->SetTitleSize(0.1);
		ts2[dir]->SetTitleOffset(0);

		ts2[dir]->GetXaxis()->SetTitle("Digitizer time-sampling cell");
		ts2[dir]->GetXaxis()->CenterTitle(true);
		ts2[dir]->GetXaxis()->SetTitleSize(0.055);
		ts2[dir]->GetXaxis()->SetTitleOffset(1.3);
		ts2[dir]->GetXaxis()->SetLabelSize(0.045);
		ts2[dir]->GetXaxis()->SetLabelOffset(0.01);

		ts2[dir]->GetYaxis()->CenterTitle(true);
		ts2[dir]->GetYaxis()->SetTitleSize(0.055);
		ts2[dir]->GetYaxis()->SetTitleOffset(0.8);
		ts2[dir]->GetYaxis()->SetLabelSize(0.045);
		ts2[dir]->GetYaxis()->SetLabelOffset(0.01);

		ts2[dir]->GetZaxis()->SetTitle("Charge [arb.u.]");
		ts2[dir]->GetZaxis()->CenterTitle(true);
		ts2[dir]->GetZaxis()->SetTitleSize(0.055);
		ts2[dir]->GetZaxis()->SetTitleOffset(0.75);
		ts2[dir]->GetZaxis()->SetLabelSize(0.045);
		ts2[dir]->GetZaxis()->SetLabelOffset(0.01);

		gPad->Update(); // updates list of functions to get TPaletteAxis
		ts2[dir]->GetListOfFunctions()->ls();
		auto pa2 = (TPaletteAxis*)ts2[dir]->GetListOfFunctions()->FindObject("palette");
		pa2->SetX2NDC(0.92);
		pa2->SetX1NDC(0.87);
		pa2->SetY2NDC(1 - 0.10);
		pa2->SetY1NDC(0.15);
		ts2[dir]->GetXaxis()->SetNdivisions(510);
		ts2[dir]->GetYaxis()->SetNdivisions(510);
		gPad->SetRightMargin(0.15);
		gPad->SetLeftMargin(0.10);
		gPad->SetTopMargin(0.10);
		gPad->SetBottomMargin(0.15);
		//    gPad->ReDrawAxis();
		gPad->Update();
		gPad->Modified();
	}
	c2->Modified();
	c2->Update();
	c2->SetSelected(c2.get());

	// PNG - works fine (transparency is preserved)
	c2->Print(Form("%s__output_UVW_vs_time.png", prefix.c_str()), "png");

	// C - works fine (transparency & colors are preserved)
	//  c2->Print(Form("%s__output_UVW_vs_time.C",prefix.c_str()), "cxx");

	// PS, EPS - produce opaque boxes (transparency is broken!) 
	//  c2->Print(Form("%s__output_UVW_vs_time.eps",prefix.c_str()), "eps");

	// ROOT - to read stored canvas with GL object use:
	// root -l YOUR_FILE.root
	// _file0->cd()
	// gStyle->SetCanvasPreferGL(kTRUE);
	// gStyle->SetPalette(1);
	// c2->Draw();
	c2->Print(Form("%s__output_UVW_vs_time.root", prefix.c_str()));

	gStyle->SetCanvasPreferGL(false);
	gStyle->SetOptTitle(false);
	set_custom_palette(palette, 99);
	auto c3 = std::make_unique<TCanvas>("c3", "c3", 900, (int)(900 * 3.0));
	c3->Divide(1, 3);
	c3->cd();
	std::map<direction, std::shared_ptr<TH2D>> ts3;

	zmax = 0.0;
	zmin = 0.0;

	for (auto dir : dirs) {
		c3->cd(int(dir) + 1);
		//    ts3[dir] = (TH2D*) p->DrawStripVsTime(dir, "LEGO2 FB");
		ts3[dir] = p->GetStripVsTime_TH2D(dir);
		if (!ts3[dir]) {
			std::cerr << "ERROR: Strip vs time TH2D[" << dir << "] is NULL !!!" << std::endl;
			return;
		}
		ts3[dir]->Draw("LEGO2 FB");
		std::cout << "dir=" << dir << ": ptr=" << ts3[dir] << ", integral=" << ts3[dir]->Integral() << std::endl;
		if (ts3[dir]->GetMaximum() > zmax) zmax = ts3[dir]->GetMaximum();
		if (ts3[dir]->GetMinimum() < zmin) zmin = ts3[dir]->GetMinimum();
	}
	for (auto dir : dirs) {
		c3->cd(int(dir) + 1);

		ts3[dir]->SetLineColorAlpha(kWhite, 0.01);
		ts3[dir]->SetContour(30);
		ts3[dir]->SetMinimum(zmin);
		ts3[dir]->SetMaximum(zmax);

		ts3[dir]->SetTitle("Charge(time) per strip");
		ts3[dir]->SetTitleSize(0.1);
		ts3[dir]->SetTitleOffset(0);

		ts3[dir]->GetXaxis()->SetTitle("Digitizer time-sampling cell");
		ts3[dir]->GetXaxis()->CenterTitle(true);
		ts3[dir]->GetXaxis()->SetTitleSize(0.055);
		ts3[dir]->GetXaxis()->SetTitleOffset(2);
		ts3[dir]->GetXaxis()->SetLabelSize(0.045);
		ts3[dir]->GetXaxis()->SetLabelOffset(0.01);

		ts3[dir]->GetYaxis()->CenterTitle(true);
		ts3[dir]->GetYaxis()->SetTitleSize(0.055);
		ts3[dir]->GetYaxis()->SetTitleOffset(1.5);
		ts3[dir]->GetYaxis()->SetLabelSize(0.045);
		ts3[dir]->GetYaxis()->SetLabelOffset(0.01);

		ts3[dir]->GetZaxis()->SetTitle("Charge [arb.u.]");
		ts3[dir]->GetZaxis()->CenterTitle(true);
		ts3[dir]->GetZaxis()->SetTitleSize(0.055);
		ts3[dir]->GetZaxis()->SetTitleOffset(1);
		ts3[dir]->GetZaxis()->SetLabelSize(0.045);
		ts3[dir]->GetZaxis()->SetLabelOffset(0.01);

		gPad->SetPhi(15.0); // deg
		gPad->SetTheta(50.0); // deg
		gPad->Update(); // updates list of functions to get TPaletteAxis

	//    ts3[dir]->GetListOfFunctions()->ls();
	//    TPaletteAxis *pa3 = (TPaletteAxis*)ts3[dir]->GetListOfFunctions()->FindObject("palette"); // works with: COLZ, LEGOZ
	//    pa3->SetX2NDC(0.92);
	//    pa3->SetX1NDC(0.87);
	//    pa3->SetY2NDC(1-0.10);
	//    pa3->SetY1NDC(0.15);

		ts3[dir]->GetXaxis()->SetNdivisions(510);
		ts3[dir]->GetYaxis()->SetNdivisions(510);
		ts3[dir]->GetZaxis()->SetNdivisions(208);
		gPad->SetRightMargin(0.10);
		gPad->SetLeftMargin(0.10);
		gPad->SetTopMargin(0.10);
		gPad->SetBottomMargin(0.15);
		//    gPad->ReDrawAxis();
		gPad->Update();
		gPad->Modified();
	}
	c3->Modified();
	c3->Update();
	c3->SetSelected(c3.get());

	// PNG - works fine (transparency is preserved)
	c3->Print(Form("%s__output_UVW_vs_time2.png", prefix.c_str()), "png");

	// C - works fine (transparency & colors are preserved)
	//  c3->Print(Form("%s__output_UVW_vs_time2.C",prefix.c_str()), "cxx");

	// PS, EPS - produce opaque boxes (transparency is broken!) 
	//  c3->Print(Form("%s__output_UVW_vs_time2.eps",prefix.c_str()), "eps");

	// ROOT - to read stored canvas with GL object use:
	// root -l YOUR_FILE.root
	// _file0->cd()
	// gStyle->SetCanvasPreferGL(kTRUE);
	// gStyle->SetPalette(1);
	// c3->Draw();
	c3->Print(Form("%s__output_UVW_vs_time2.root", prefix.c_str()));


	gStyle->SetCanvasPreferGL(false);
	gStyle->SetOptTitle(false);
	set_custom_palette(palette); // back to 255 contours
	auto c4 = std::make_unique<TCanvas>("c4", "c4", 900, (int)(900 * 3.0));
	c4->Divide(1, 3);
	c4->cd();
	std::map<direction, TMultiGraph> tmg;

	zmin = 0.0;
	zmax = 0.0;

	for (auto dir : dirs) {
		c4->cd(int(dir) + 1);

		for (int istrip = 1; istrip <= Geometry().GetDirNstrips(dir); istrip++) {
			TGraph* tg = new TGraph(ts2[dir]->ProjectionX(Form("%s%d", Geometry().GetDirName(dir).c_str(), istrip), istrip, istrip));
			std::cout << "dir=" << dir << ", strip=" << istrip << ": ptr=" << tg << ", integral_tgraph=" << tg->Integral()
				<< ", integral_sliceX=" << ts2[dir]->Integral(1, ts2[dir]->GetNbinsX(), istrip, istrip) << std::endl;

			TPaletteAxis* pa2 = (TPaletteAxis*)ts2[dir]->GetListOfFunctions()->FindObject("palette");
			int ci = kBlack;
			if (pa2) {
				ci = pa2->GetValueColor((Double_t)(ts2[dir]->Integral(1, ts2[dir]->GetNbinsX(), istrip, istrip)));
			}
			tg->SetLineWidth(2);
			tg->SetLineColor(ci);
			//      tg->SetLineColorAlpha(ci,0.5);
			tmg[dir].Add(tg, "L");
		}

		tmg[dir].Draw("A");

		if (tmg[dir].GetHistogram()->GetMaximum() > zmax) zmax = tmg[dir].GetHistogram()->GetMaximum();
		if (tmg[dir].GetHistogram()->GetMinimum() < zmin) zmin = tmg[dir].GetHistogram()->GetMinimum();
	}
	for (auto dir : dirs) {
		c4->cd(int(dir) + 1);

		tmg[dir].GetHistogram()->SetMaximum(zmax);
		tmg[dir].GetHistogram()->SetMinimum(zmin);

		tmg[dir].SetTitle("Charge(time) per strip");
		//    tmg[dir].SetTitleSize(0.1);
		//    tmg[dir].SetTitleOffset(0);

		tmg[dir].GetXaxis()->SetTitle("Digitizer time-sampling cell");
		tmg[dir].GetXaxis()->CenterTitle(true);
		tmg[dir].GetXaxis()->SetTitleSize(0.055);
		tmg[dir].GetXaxis()->SetTitleOffset(1.3);
		tmg[dir].GetXaxis()->SetLabelSize(0.045);
		tmg[dir].GetXaxis()->SetLabelOffset(0.01);

		tmg[dir].GetYaxis()->SetTitle("Charge [arb.u.]");
		tmg[dir].GetYaxis()->CenterTitle(true);
		tmg[dir].GetYaxis()->SetTitleSize(0.055);
		tmg[dir].GetYaxis()->SetTitleOffset(0.8);
		tmg[dir].GetYaxis()->SetLabelSize(0.045);
		tmg[dir].GetYaxis()->SetLabelOffset(0.01);

		gPad->Update(); // updates list of functions to get TPaletteAxis

	//    tmg[dir].GetListOfFunctions()->ls();
	//    TPaletteAxis *pa4 = (TPaletteAxis*)tmg[dir].GetListOfFunctions()->FindObject("palette"); // works with: COLZ LEGOZ
	//    pa4->SetX2NDC(0.92);
	//    pa4->SetX1NDC(0.87);
	//    pa4->SetY2NDC(1-0.10);
	//    pa4->SetY1NDC(0.15);

		tmg[dir].GetXaxis()->SetNdivisions(510);
		tmg[dir].GetYaxis()->SetNdivisions(510);
		gPad->SetRightMargin(0.05);
		gPad->SetLeftMargin(0.10);
		gPad->SetTopMargin(0.05);
		gPad->SetBottomMargin(0.15);
		//    gPad->ReDrawAxis();
		gPad->Update();
		gPad->Modified();
	}
	c4->Modified();
	c4->Update();
	c4->SetSelected(c4.get());

	// PNG - works fine (transparency is preserved)
	c4->Print(Form("%s__output_UVW_vs_time3.png", prefix.c_str()), "png");

	// C - works fine (transparency & colors are preserved)
	//  c4->Print(Form("%s__output_UVW_vs_time3.C",prefix.c_str()), "cxx");

	// PS, EPS - produce opaque boxes (transparency is broken!) 
	//  c4->Print(Form("%s__output_UVW_vs_time3.eps",prefix.c_str()), "eps");

	// ROOT - to read stored canvas with GL object use:
	// root -l YOUR_FILE.root
	// _file0->cd()
	// gStyle->SetCanvasPreferGL(kTRUE);
	// gStyle->SetPalette(1);
	// c4->Draw();
	c4->Print(Form("%s__output_UVW_vs_time3.root", prefix.c_str()));


	gStyle->SetCanvasPreferGL(false);
	gStyle->SetOptTitle(false);
	set_custom_palette(palette); // back to 255 contours
	auto c5 = std::make_unique<TCanvas>("c5", "c5", 900, (int)(900 * 3.0));
	c5->Divide(1, 3);
	c5->cd();
	std::map<direction, std::shared_ptr<TH1D>> ts;
	for (auto dir : dirs) {
		c5->cd(int(dir) + 1);
		//    ts[dir] = (std::shared_ptr<TH1D>) p->DrawStripProfile(dir);
		ts[dir] = p->GetStripProfile_TH1D(dir);
		if (ts[dir] == nullptr) {
			std::cerr << "ERROR: Strip profile TH1D[" << dir << "] is NULL !!!" << std::endl;
			return;
		}
		ts[dir]->Draw("HIST");
		ts[dir]->SetLineWidth(2);
		std::cout << "dir=" << dir << ": ptr=" << ts[dir] << ", integral=" << ts[dir]->Integral() << std::endl;

		ts[dir]->SetTitle("Time-integrated charge per strip");
		ts[dir]->SetTitleSize(0.1);
		ts[dir]->SetTitleOffset(0);

		ts[dir]->GetXaxis()->CenterTitle(true);
		ts[dir]->GetXaxis()->SetTitleSize(0.055);
		ts[dir]->GetXaxis()->SetTitleOffset(1.3);
		ts[dir]->GetXaxis()->SetLabelSize(0.045);
		ts[dir]->GetXaxis()->SetLabelOffset(0.01);

		ts[dir]->GetYaxis()->SetTitle("Charge [arb.u.]");
		ts[dir]->GetYaxis()->CenterTitle(true);
		ts[dir]->GetYaxis()->SetTitleSize(0.055);
		ts[dir]->GetYaxis()->SetTitleOffset(0.8);
		ts[dir]->GetYaxis()->SetLabelSize(0.045);
		ts[dir]->GetYaxis()->SetLabelOffset(0.01);

		ts[dir]->GetXaxis()->SetNdivisions(510);
		ts[dir]->GetYaxis()->SetNdivisions(510);

		gPad->SetRightMargin(0.05);
		gPad->SetLeftMargin(0.10);
		gPad->SetTopMargin(0.05);
		gPad->SetBottomMargin(0.15);

		//    gPad->ReDrawAxis();
		gPad->Update();
		gPad->Modified();
	}
	c5->Modified();
	c5->Update();
	c5->SetSelected(c5.get());

	// PNG - works fine (transparency is preserved)
	c5->Print(Form("%s__output_UVW.png", prefix.c_str()), "png");

	// C - works fine (transparency & colors are preserved)
	//  c5->Print(Form("%s__output_UVW.C",prefix.c_str()), "cxx");

	// PS, EPS - produce opaque boxes (transparency is broken!) 
	//  c5->Print(Form("%s__output_UVW.eps",prefix.c_str()), "eps");

	// ROOT - to read stored canvas with GL object use:
	// root -l YOUR_FILE.root
	// _file0->cd()
	// gStyle->SetCanvasPreferGL(kTRUE);
	// gStyle->SetPalette(1);
	// c5->Draw();
	c5->Print(Form("%s__output_UVW.root", prefix.c_str()));

}


//_________________________
// load TH3D from ROOT file
//
TH3D* load_TH3D(const char* input_fname, const char* input_hname) {

	TFile* fin = NULL;
	if (input_fname) fin = new TFile(input_fname, "OLD");
	if (!fin) return NULL;
	fin->cd();
	fin->ls();

	TH3D* h3_orig = (TH3D*)(fin->Get(input_hname));  // also works with TH3F
	if (!h3_orig || h3_orig->GetDimension() != 3) return NULL;

	TH3D* h3 = (TH3D*)(h3_orig->Clone(Form("%s_copy", h3_orig->GetName()))); // new TH3D(*h3_orig);

	std::cout << "load_TH3D: Success, ptr=" << h3 << ", dim=" << h3->GetDimension()
		<< ", min=" << h3->GetMinimum() << ", max=" << h3->GetMaximum()
		<< std::endl << std::flush;

	return h3; // return pointer to a copy, file fin will be closed automatically
}

//___________________________________________________________
// check if binnig of two TH3D histograms is exactly the same
//
bool compare_TH3D_bins(TH3D* h1, TH3D* h2) {
	if (!h1 || !h2 || h1->GetDimension() != 3 || h2->GetDimension() != 3) return false; // dimension mismatch

	std::cout << h1->GetNbinsX()
		<< " [" << h1->GetXaxis()->GetXmin() << ", " << h1->GetXaxis()->GetXmax() << "] vs "
		<< h2->GetNbinsX()
		<< " [" << h2->GetXaxis()->GetXmin() << ", " << h2->GetXaxis()->GetXmax() << "]" << std::endl;
	std::cout << h1->GetNbinsY()
		<< " [" << h1->GetYaxis()->GetXmin() << ", " << h1->GetYaxis()->GetXmax() << "] vs "
		<< h2->GetNbinsY()
		<< " [" << h2->GetYaxis()->GetXmin() << ", " << h2->GetYaxis()->GetXmax() << "]" << std::endl;
	std::cout << h1->GetNbinsZ()
		<< " [" << h1->GetZaxis()->GetXmin() << ", " << h1->GetZaxis()->GetXmax() << "] vs "
		<< h2->GetNbinsZ()
		<< " [" << h2->GetZaxis()->GetXmin() << ", " << h2->GetZaxis()->GetXmax() << "]" << std::endl;

	return (h2->GetXaxis()->GetXmax() - h2->GetXaxis()->GetXmin()) / h2->GetNbinsX() ==
		(h1->GetXaxis()->GetXmax() - h1->GetXaxis()->GetXmin()) / h1->GetNbinsX() &&
		(h2->GetYaxis()->GetXmax() - h2->GetYaxis()->GetXmin()) / h2->GetNbinsY() ==
		(h1->GetYaxis()->GetXmax() - h1->GetYaxis()->GetXmin()) / h1->GetNbinsY() &&
		(h2->GetZaxis()->GetXmax() - h2->GetZaxis()->GetXmin()) / h2->GetNbinsZ() ==
		(h1->GetZaxis()->GetXmax() - h1->GetZaxis()->GetXmin()) / h1->GetNbinsZ(); // check for bin size mismatch
}

//__________________________________________________________
// creates new TH3D with rescaled XYZ axes of TH3D histogram 
//
TH3D* rescale_TH3D_axes(TH3D* h3_orig, double convert_to_mm_factor) {

	TH3D* h3_scaled = NULL;
	if (convert_to_mm_factor != 1.0) {

		std::cout << h3_orig->GetNbinsX() << " "
			<< h3_orig->GetXaxis()->GetXmin() << " "
			<< h3_orig->GetXaxis()->GetXmax() << std::endl;
		std::cout << h3_orig->GetNbinsY() << " "
			<< h3_orig->GetYaxis()->GetXmin() << " "
			<< h3_orig->GetYaxis()->GetXmax() << std::endl;
		std::cout << h3_orig->GetNbinsZ() << " "
			<< h3_orig->GetZaxis()->GetXmin() << " "
			<< h3_orig->GetZaxis()->GetXmax() << std::endl;

		h3_scaled = new TH3D(Form("%s_scaled", h3_orig->GetName()),
			h3_orig->GetTitle(),
			h3_orig->GetNbinsX(),
			h3_orig->GetXaxis()->GetXmin() * convert_to_mm_factor,
			h3_orig->GetXaxis()->GetXmax() * convert_to_mm_factor,
			h3_orig->GetNbinsY(),
			h3_orig->GetYaxis()->GetXmin() * convert_to_mm_factor,
			h3_orig->GetYaxis()->GetXmax() * convert_to_mm_factor,
			h3_orig->GetNbinsZ(),
			h3_orig->GetZaxis()->GetXmin() * convert_to_mm_factor,
			h3_orig->GetZaxis()->GetXmax() * convert_to_mm_factor);
		for (int ix = 1; ix <= h3_scaled->GetNbinsX(); ix++) {
			const double x = h3_scaled->GetXaxis()->GetBinCenter(ix);
			for (int iy = 1; iy <= h3_scaled->GetNbinsY(); iy++) {
				const double y = h3_scaled->GetYaxis()->GetBinCenter(iy);
				for (int iz = 1; iz <= h3_scaled->GetNbinsZ(); iz++) {
					const long index = h3_scaled->GetBin(ix, iy, iz);
					const double val = h3_orig->GetBinContent(index);
					if (val == 0.0) continue;
					h3_scaled->Fill(x, y, h3_scaled->GetZaxis()->GetBinCenter(iz), val);
				}
			}
		}
	}
	else {
		h3_scaled = (TH3D*)(h3_orig->Clone(Form("%s_scaled", h3_orig->GetName()))); // new TH3D(*h3_orig) ; 
	}
	return h3_scaled;
}

//_________________________________________
// performs operation: h1 := h1 + weight*h2
// where: 
// - h1, h2 must have same bin widths
// - h1, h2 can have different axis ranges
//
bool add_to_TH3D(TH3D* h1, TH3D* h2, double weight) {

	if (h1 == nullptr) return false; // first histogram is required
	if (weight == 0.0 || h2 == nullptr) return true; // nothing to be changed!
	if (!compare_TH3D_bins(h1, h2)) return false; // different bin widths

	if (!h1->Add(h2, weight)) {	// different ranges
		for (int ix = 1; ix <= h2->GetNbinsX(); ix++) {
			const double x = h2->GetXaxis()->GetBinCenter(ix);
			for (int iy = 1; iy <= h2->GetNbinsY(); iy++) {
				const double y = h2->GetYaxis()->GetBinCenter(iy);
				for (int iz = 1; iz <= h2->GetNbinsZ(); iz++) {
					const long index = h2->GetBin(ix, iy, iz);
					const double val = h2->GetBinContent(index);
					if (val == 0.0) continue;
					h1->Fill(x, y, h2->GetZaxis()->GetBinCenter(iz), val * weight);
				}
			}
		}
	}
	return true;
}

Double_t my_transfer_function(const Double_t* x, const Double_t* param)
{
	// Requires 2 params:
	// - Bin values from param[0] to param[1]
	if (param) {

		double y = (*x - param[0]) / (param[1] - param[0]); // within [0 ,1] range

		if (y <= 0.01) return 0.0;  // fully transparent
		double val = 0.02 + 0.98 * pow(y, 2);     // result is always within ]0,1] range for 0<y<1
		return val;

	}
	return 0.0;
}

// Example of custom color palette taken from [1].
// [1] http://ultrahigh.org/2007/08/making-pretty-root-color-palettes/ -- acccess: 4 Nov 2017
//
Double_t my_transfer_function3(const Double_t* x, const Double_t* param)
{
	// Requires 3 params:
	// - Bin values from param[0] to param[1]
	// - Power = param[2]
	if (param) {

		double y = (*x - param[0]) / (param[1] - param[0]); // within [0 ,1] range
		if (y <= 5e-3) return 0.0;  // fully transparent
		double val = 0.02 + 0.98 * pow(y, param[2]);     // result is always within ]0,1] range for 0<y<1
		/*
		if(y <= 0.01) return 0.0;  // fully transparent
		double val = 0.02+0.98*pow(y,param[2]);     // result is always within ]0,1] range for 0<y<1
		*/
		return val;

	}
	return 0.0;
}

void set_custom_rainbow_palette(const Int_t NCont, const Float_t alpha)
{
	const Int_t NRGBs = 5;

	Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
	Double_t red[NRGBs] = { 0.00, 0.00, 0.87, 1.00, 0.51 };
	Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
	Double_t blue[NRGBs] = { 0.51, 1.00, 0.12, 0.00, 0.00 };
	TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont, alpha);
	gStyle->SetNumberContours(NCont);
}

// Imported from ROOT 6.04 TColor::SetPalette
void set_custom_palette(const Int_t index, Int_t NCont, const Float_t alpha)
{
	const Int_t NRGBs = 9;
	Double_t stops[NRGBs] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000 };
	Int_t Idx;

	switch (index) {
		// Deep Sea
	case 51:
	{
		Double_t red[NRGBs] = { 0. / 255.,  9. / 255., 13. / 255., 17. / 255., 24. / 255.,  32. / 255.,  27. / 255.,  25. / 255.,  29. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  0. / 255.,  0. / 255.,  2. / 255., 37. / 255.,  74. / 255., 113. / 255., 160. / 255., 221. / 255. };
		Double_t blue[NRGBs] = { 28. / 255., 42. / 255., 59. / 255., 78. / 255., 98. / 255., 129. / 255., 154. / 255., 184. / 255., 221. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Grey Scale
	case 52:
	{
		Double_t red[NRGBs] = { 0. / 255., 32. / 255., 64. / 255., 96. / 255., 128. / 255., 160. / 255., 192. / 255., 224. / 255., 255. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 32. / 255., 64. / 255., 96. / 255., 128. / 255., 160. / 255., 192. / 255., 224. / 255., 255. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 32. / 255., 64. / 255., 96. / 255., 128. / 255., 160. / 255., 192. / 255., 224. / 255., 255. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Dark Body Radiator
	case 53:
	{
		Double_t red[NRGBs] = { 0. / 255., 45. / 255., 99. / 255., 156. / 255., 212. / 255., 230. / 255., 237. / 255., 234. / 255., 242. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  0. / 255.,  0. / 255.,  45. / 255., 101. / 255., 168. / 255., 238. / 255., 238. / 255., 243. / 255. };
		Double_t blue[NRGBs] = { 0. / 255.,  1. / 255.,  1. / 255.,   3. / 255.,   9. / 255.,   8. / 255.,  11. / 255.,  95. / 255., 230. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Two-color hue (dark blue through neutral gray to bright yellow)
	case 54:
	{
		Double_t red[NRGBs] = { 0. / 255.,  22. / 255., 44. / 255., 68. / 255., 93. / 255., 124. / 255., 160. / 255., 192. / 255., 237. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  16. / 255., 41. / 255., 67. / 255., 93. / 255., 125. / 255., 162. / 255., 194. / 255., 241. / 255. };
		Double_t blue[NRGBs] = { 97. / 255., 100. / 255., 99. / 255., 99. / 255., 93. / 255.,  68. / 255.,  44. / 255.,  26. / 255.,  74. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Rain Bow
	case 55:
	{
		Double_t red[NRGBs] = { 0. / 255.,   5. / 255.,  15. / 255.,  35. / 255., 102. / 255., 196. / 255., 208. / 255., 199. / 255., 110. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  48. / 255., 124. / 255., 192. / 255., 206. / 255., 226. / 255.,  97. / 255.,  16. / 255.,   0. / 255. };
		Double_t blue[NRGBs] = { 99. / 255., 142. / 255., 198. / 255., 201. / 255.,  90. / 255.,  22. / 255.,  13. / 255.,   8. / 255.,   2. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Inverted Dark Body Radiator
	case 56:
	{
		Double_t red[NRGBs] = { 242. / 255., 234. / 255., 237. / 255., 230. / 255., 212. / 255., 156. / 255., 99. / 255., 45. / 255., 0. / 255. };
		Double_t green[NRGBs] = { 243. / 255., 238. / 255., 238. / 255., 168. / 255., 101. / 255.,  45. / 255.,  0. / 255.,  0. / 255., 0. / 255. };
		Double_t blue[NRGBs] = { 230. / 255.,  95. / 255.,  11. / 255.,   8. / 255.,   9. / 255.,   3. / 255.,  1. / 255.,  1. / 255., 0. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Bird
	case 57:
	{
		Double_t red[NRGBs] = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764 };
		Double_t green[NRGBs] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832 };
		Double_t blue[NRGBs] = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539 };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Cubehelix
	case 58:
	{
		Double_t red[NRGBs] = { 0.0000, 0.0956, 0.0098, 0.2124, 0.6905, 0.9242, 0.7914, 0.7596, 1.0000 };
		Double_t green[NRGBs] = { 0.0000, 0.1147, 0.3616, 0.5041, 0.4577, 0.4691, 0.6905, 0.9237, 1.0000 };
		Double_t blue[NRGBs] = { 0.0000, 0.2669, 0.3121, 0.1318, 0.2236, 0.6741, 0.9882, 0.9593, 1.0000 };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Green Red Violet
	case 59:
	{
		Double_t red[NRGBs] = { 13. / 255., 23. / 255., 25. / 255., 63. / 255., 76. / 255., 104. / 255., 137. / 255., 161. / 255., 206. / 255. };
		Double_t green[NRGBs] = { 95. / 255., 67. / 255., 37. / 255., 21. / 255.,  0. / 255.,  12. / 255.,  35. / 255.,  52. / 255.,  79. / 255. };
		Double_t blue[NRGBs] = { 4. / 255.,  3. / 255.,  2. / 255.,  6. / 255., 11. / 255.,  22. / 255.,  49. / 255.,  98. / 255., 208. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Blue Red Yellow
	case 60:
	{
		Double_t red[NRGBs] = { 0. / 255.,  61. / 255.,  89. / 255., 122. / 255., 143. / 255., 160. / 255., 185. / 255., 204. / 255., 231. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,   0. / 255.,   0. / 255.,   0. / 255.,  14. / 255.,  37. / 255.,  72. / 255., 132. / 255., 235. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 140. / 255., 224. / 255., 144. / 255.,   4. / 255.,   5. / 255.,   6. / 255.,   9. / 255.,  13. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Ocean
	case 61:
	{
		Double_t red[NRGBs] = { 14. / 255.,  7. / 255.,  2. / 255.,  0. / 255.,  5. / 255.,  11. / 255.,  55. / 255., 131. / 255., 229. / 255. };
		Double_t green[NRGBs] = { 105. / 255., 56. / 255., 26. / 255.,  1. / 255., 42. / 255.,  74. / 255., 131. / 255., 171. / 255., 229. / 255. };
		Double_t blue[NRGBs] = { 2. / 255., 21. / 255., 35. / 255., 60. / 255., 92. / 255., 113. / 255., 160. / 255., 185. / 255., 229. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Color Printable On Grey
	case 62:
	{
		Double_t red[NRGBs] = { 0. / 255.,   0. / 255.,   0. / 255.,  70. / 255., 148. / 255., 231. / 255., 235. / 255., 237. / 255., 244. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,   0. / 255.,   0. / 255.,   0. / 255.,   0. / 255.,  69. / 255.,  67. / 255., 216. / 255., 244. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 102. / 255., 228. / 255., 231. / 255., 177. / 255., 124. / 255., 137. / 255.,  20. / 255., 244. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Alpine
	case 63:
	{
		Double_t red[NRGBs] = { 50. / 255., 56. / 255., 63. / 255., 68. / 255.,  93. / 255., 121. / 255., 165. / 255., 192. / 255., 241. / 255. };
		Double_t green[NRGBs] = { 66. / 255., 81. / 255., 91. / 255., 96. / 255., 111. / 255., 128. / 255., 155. / 255., 189. / 255., 241. / 255. };
		Double_t blue[NRGBs] = { 97. / 255., 91. / 255., 75. / 255., 65. / 255.,  77. / 255., 103. / 255., 143. / 255., 167. / 255., 217. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Aquamarine
	case 64:
	{
		Double_t red[NRGBs] = { 145. / 255., 166. / 255., 167. / 255., 156. / 255., 131. / 255., 114. / 255., 101. / 255., 112. / 255., 132. / 255. };
		Double_t green[NRGBs] = { 158. / 255., 178. / 255., 179. / 255., 181. / 255., 163. / 255., 154. / 255., 144. / 255., 152. / 255., 159. / 255. };
		Double_t blue[NRGBs] = { 190. / 255., 199. / 255., 201. / 255., 192. / 255., 176. / 255., 169. / 255., 160. / 255., 166. / 255., 190. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Army
	case 65:
	{
		Double_t red[NRGBs] = { 93. / 255.,   91. / 255.,  99. / 255., 108. / 255., 130. / 255., 125. / 255., 132. / 255., 155. / 255., 174. / 255. };
		Double_t green[NRGBs] = { 126. / 255., 124. / 255., 128. / 255., 129. / 255., 131. / 255., 121. / 255., 119. / 255., 153. / 255., 173. / 255. };
		Double_t blue[NRGBs] = { 103. / 255.,  94. / 255.,  87. / 255.,  85. / 255.,  80. / 255.,  85. / 255., 107. / 255., 120. / 255., 146. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Atlantic
	case 66:
	{
		Double_t red[NRGBs] = { 24. / 255., 40. / 255., 69. / 255.,  90. / 255., 104. / 255., 114. / 255., 120. / 255., 132. / 255., 103. / 255. };
		Double_t green[NRGBs] = { 29. / 255., 52. / 255., 94. / 255., 127. / 255., 150. / 255., 162. / 255., 159. / 255., 151. / 255., 101. / 255. };
		Double_t blue[NRGBs] = { 29. / 255., 52. / 255., 96. / 255., 132. / 255., 162. / 255., 181. / 255., 184. / 255., 186. / 255., 131. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Aurora
	case 67:
	{
		Double_t red[NRGBs] = { 46. / 255., 38. / 255., 61. / 255., 92. / 255., 113. / 255., 121. / 255., 132. / 255., 150. / 255., 191. / 255. };
		Double_t green[NRGBs] = { 46. / 255., 36. / 255., 40. / 255., 69. / 255., 110. / 255., 135. / 255., 131. / 255.,  92. / 255.,  34. / 255. };
		Double_t blue[NRGBs] = { 46. / 255., 80. / 255., 74. / 255., 70. / 255.,  81. / 255., 105. / 255., 165. / 255., 211. / 255., 225. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Avocado
	case 68:
	{
		Double_t red[NRGBs] = { 0. / 255.,  4. / 255., 12. / 255.,  30. / 255.,  52. / 255., 101. / 255., 142. / 255., 190. / 255., 237. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 40. / 255., 86. / 255., 121. / 255., 140. / 255., 172. / 255., 187. / 255., 213. / 255., 240. / 255. };
		Double_t blue[NRGBs] = { 0. / 255.,  9. / 255., 14. / 255.,  18. / 255.,  21. / 255.,  23. / 255.,  27. / 255.,  35. / 255., 101. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Beach
	case 69:
	{
		Double_t red[NRGBs] = { 198. / 255., 206. / 255., 206. / 255., 211. / 255., 198. / 255., 181. / 255., 161. / 255., 171. / 255., 244. / 255. };
		Double_t green[NRGBs] = { 103. / 255., 133. / 255., 150. / 255., 172. / 255., 178. / 255., 174. / 255., 163. / 255., 175. / 255., 244. / 255. };
		Double_t blue[NRGBs] = { 49. / 255.,  54. / 255.,  55. / 255.,  66. / 255.,  91. / 255., 130. / 255., 184. / 255., 224. / 255., 244. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Black Body
	case 70:
	{
		Double_t red[NRGBs] = { 243. / 255., 243. / 255., 240. / 255., 240. / 255., 241. / 255., 239. / 255., 186. / 255., 151. / 255., 129. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  46. / 255.,  99. / 255., 149. / 255., 194. / 255., 220. / 255., 183. / 255., 166. / 255., 147. / 255. };
		Double_t blue[NRGBs] = { 6. / 255.,   8. / 255.,  36. / 255.,  91. / 255., 169. / 255., 235. / 255., 246. / 255., 240. / 255., 233. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Blue Green Yellow
	case 71:
	{
		Double_t red[NRGBs] = { 22. / 255., 19. / 255.,  19. / 255.,  25. / 255.,  35. / 255.,  53. / 255.,  88. / 255., 139. / 255., 210. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 32. / 255.,  69. / 255., 108. / 255., 135. / 255., 159. / 255., 183. / 255., 198. / 255., 215. / 255. };
		Double_t blue[NRGBs] = { 77. / 255., 96. / 255., 110. / 255., 116. / 255., 110. / 255., 100. / 255.,  90. / 255.,  78. / 255.,  70. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Brown Cyan
	case 72:
	{
		Double_t red[NRGBs] = { 68. / 255., 116. / 255., 165. / 255., 182. / 255., 189. / 255., 180. / 255., 145. / 255., 111. / 255.,  71. / 255. };
		Double_t green[NRGBs] = { 37. / 255.,  82. / 255., 135. / 255., 178. / 255., 204. / 255., 225. / 255., 221. / 255., 202. / 255., 147. / 255. };
		Double_t blue[NRGBs] = { 16. / 255.,  55. / 255., 105. / 255., 147. / 255., 196. / 255., 226. / 255., 232. / 255., 224. / 255., 178. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// CMYK
	case 73:
	{
		Double_t red[NRGBs] = { 61. / 255.,  99. / 255., 136. / 255., 181. / 255., 213. / 255., 225. / 255., 198. / 255., 136. / 255., 24. / 255. };
		Double_t green[NRGBs] = { 149. / 255., 140. / 255.,  96. / 255.,  83. / 255., 132. / 255., 178. / 255., 190. / 255., 135. / 255., 22. / 255. };
		Double_t blue[NRGBs] = { 214. / 255., 203. / 255., 168. / 255., 135. / 255., 110. / 255., 100. / 255., 111. / 255., 113. / 255., 22. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Candy
	case 74:
	{
		Double_t red[NRGBs] = { 76. / 255., 120. / 255., 156. / 255., 183. / 255., 197. / 255., 180. / 255., 162. / 255., 154. / 255., 140. / 255. };
		Double_t green[NRGBs] = { 34. / 255.,  35. / 255.,  42. / 255.,  69. / 255., 102. / 255., 137. / 255., 164. / 255., 188. / 255., 197. / 255. };
		Double_t blue[NRGBs] = { 64. / 255.,  69. / 255.,  78. / 255., 105. / 255., 142. / 255., 177. / 255., 205. / 255., 217. / 255., 198. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Cherry
	case 75:
	{
		Double_t red[NRGBs] = { 37. / 255., 102. / 255., 157. / 255., 188. / 255., 196. / 255., 214. / 255., 223. / 255., 235. / 255., 251. / 255. };
		Double_t green[NRGBs] = { 37. / 255.,  29. / 255.,  25. / 255.,  37. / 255.,  67. / 255.,  91. / 255., 132. / 255., 185. / 255., 251. / 255. };
		Double_t blue[NRGBs] = { 37. / 255.,  32. / 255.,  33. / 255.,  45. / 255.,  66. / 255.,  98. / 255., 137. / 255., 187. / 255., 251. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Coffee
	case 76:
	{
		Double_t red[NRGBs] = { 79. / 255., 100. / 255., 119. / 255., 137. / 255., 153. / 255., 172. / 255., 192. / 255., 205. / 255., 250. / 255. };
		Double_t green[NRGBs] = { 63. / 255.,  79. / 255.,  93. / 255., 103. / 255., 115. / 255., 135. / 255., 167. / 255., 196. / 255., 250. / 255. };
		Double_t blue[NRGBs] = { 51. / 255.,  59. / 255.,  66. / 255.,  61. / 255.,  62. / 255.,  70. / 255., 110. / 255., 160. / 255., 250. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Dark Rain Bow
	case 77:
	{
		Double_t red[NRGBs] = { 43. / 255.,  44. / 255., 50. / 255.,  66. / 255., 125. / 255., 172. / 255., 178. / 255., 155. / 255., 157. / 255. };
		Double_t green[NRGBs] = { 63. / 255.,  63. / 255., 85. / 255., 101. / 255., 138. / 255., 163. / 255., 122. / 255.,  51. / 255.,  39. / 255. };
		Double_t blue[NRGBs] = { 121. / 255., 101. / 255., 58. / 255.,  44. / 255.,  47. / 255.,  55. / 255.,  57. / 255.,  44. / 255.,  43. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Dark Terrain
	case 78:
	{
		Double_t red[NRGBs] = { 0. / 255., 41. / 255., 62. / 255., 79. / 255., 90. / 255., 87. / 255., 99. / 255., 140. / 255., 228. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 57. / 255., 81. / 255., 93. / 255., 85. / 255., 70. / 255., 71. / 255., 125. / 255., 228. / 255. };
		Double_t blue[NRGBs] = { 95. / 255., 91. / 255., 91. / 255., 82. / 255., 60. / 255., 43. / 255., 44. / 255., 112. / 255., 228. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Fall
	case 79:
	{
		Double_t red[NRGBs] = { 49. / 255., 59. / 255., 72. / 255., 88. / 255., 114. / 255., 141. / 255., 176. / 255., 205. / 255., 222. / 255. };
		Double_t green[NRGBs] = { 78. / 255., 72. / 255., 66. / 255., 57. / 255.,  59. / 255.,  75. / 255., 106. / 255., 142. / 255., 173. / 255. };
		Double_t blue[NRGBs] = { 78. / 255., 55. / 255., 46. / 255., 40. / 255.,  39. / 255.,  39. / 255.,  40. / 255.,  41. / 255.,  47. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Fruit Punch
	case 80:
	{
		Double_t red[NRGBs] = { 243. / 255., 222. / 255., 201. / 255., 185. / 255., 165. / 255., 158. / 255., 166. / 255., 187. / 255., 219. / 255. };
		Double_t green[NRGBs] = { 94. / 255., 108. / 255., 132. / 255., 135. / 255., 125. / 255.,  96. / 255.,  68. / 255.,  51. / 255.,  61. / 255. };
		Double_t blue[NRGBs] = { 7. / 255.,  9. / 255.,   12. / 255.,  19. / 255.,  45. / 255.,  89. / 255., 118. / 255., 146. / 255., 118. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Fuchsia
	case 81:
	{
		Double_t red[NRGBs] = { 19. / 255., 44. / 255., 74. / 255., 105. / 255., 137. / 255., 166. / 255., 194. / 255., 206. / 255., 220. / 255. };
		Double_t green[NRGBs] = { 19. / 255., 28. / 255., 40. / 255.,  55. / 255.,  82. / 255., 110. / 255., 159. / 255., 181. / 255., 220. / 255. };
		Double_t blue[NRGBs] = { 19. / 255., 42. / 255., 68. / 255.,  96. / 255., 129. / 255., 157. / 255., 188. / 255., 203. / 255., 220. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Grey Yellow
	case 82:
	{
		Double_t red[NRGBs] = { 33. / 255., 44. / 255., 70. / 255.,  99. / 255., 140. / 255., 165. / 255., 199. / 255., 211. / 255., 216. / 255. };
		Double_t green[NRGBs] = { 38. / 255., 50. / 255., 76. / 255., 105. / 255., 140. / 255., 165. / 255., 191. / 255., 189. / 255., 167. / 255. };
		Double_t blue[NRGBs] = { 55. / 255., 67. / 255., 97. / 255., 124. / 255., 140. / 255., 166. / 255., 163. / 255., 129. / 255.,  52. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Green Brown Terrain
	case 83:
	{
		Double_t red[NRGBs] = { 0. / 255., 33. / 255., 73. / 255., 124. / 255., 136. / 255., 152. / 255., 159. / 255., 171. / 255., 223. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 43. / 255., 92. / 255., 124. / 255., 134. / 255., 126. / 255., 121. / 255., 144. / 255., 223. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 43. / 255., 68. / 255.,  76. / 255.,  73. / 255.,  64. / 255.,  72. / 255., 114. / 255., 223. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Green Pink
	case 84:
	{
		Double_t red[NRGBs] = { 5. / 255.,  18. / 255.,  45. / 255., 124. / 255., 193. / 255., 223. / 255., 205. / 255., 128. / 255., 49. / 255. };
		Double_t green[NRGBs] = { 48. / 255., 134. / 255., 207. / 255., 230. / 255., 193. / 255., 113. / 255.,  28. / 255.,   0. / 255.,  7. / 255. };
		Double_t blue[NRGBs] = { 6. / 255.,  15. / 255.,  41. / 255., 121. / 255., 193. / 255., 226. / 255., 208. / 255., 130. / 255., 49. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Island
	case 85:
	{
		Double_t red[NRGBs] = { 180. / 255., 106. / 255., 104. / 255., 135. / 255., 164. / 255., 188. / 255., 189. / 255., 165. / 255., 144. / 255. };
		Double_t green[NRGBs] = { 72. / 255., 126. / 255., 154. / 255., 184. / 255., 198. / 255., 207. / 255., 205. / 255., 190. / 255., 179. / 255. };
		Double_t blue[NRGBs] = { 41. / 255., 120. / 255., 158. / 255., 188. / 255., 194. / 255., 181. / 255., 145. / 255., 100. / 255.,  62. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Lake
	case 86:
	{
		Double_t red[NRGBs] = { 57. / 255.,  72. / 255.,  94. / 255., 117. / 255., 136. / 255., 154. / 255., 174. / 255., 192. / 255., 215. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  33. / 255.,  68. / 255., 109. / 255., 140. / 255., 171. / 255., 192. / 255., 196. / 255., 209. / 255. };
		Double_t blue[NRGBs] = { 116. / 255., 137. / 255., 173. / 255., 201. / 255., 200. / 255., 201. / 255., 203. / 255., 190. / 255., 187. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Light Temperature
	case 87:
	{
		Double_t red[NRGBs] = { 31. / 255.,  71. / 255., 123. / 255., 160. / 255., 210. / 255., 222. / 255., 214. / 255., 199. / 255., 183. / 255. };
		Double_t green[NRGBs] = { 40. / 255., 117. / 255., 171. / 255., 211. / 255., 231. / 255., 220. / 255., 190. / 255., 132. / 255.,  65. / 255. };
		Double_t blue[NRGBs] = { 234. / 255., 214. / 255., 228. / 255., 222. / 255., 210. / 255., 160. / 255., 105. / 255.,  60. / 255.,  34. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Light Terrain
	case 88:
	{
		Double_t red[NRGBs] = { 123. / 255., 108. / 255., 109. / 255., 126. / 255., 154. / 255., 172. / 255., 188. / 255., 196. / 255., 218. / 255. };
		Double_t green[NRGBs] = { 184. / 255., 138. / 255., 130. / 255., 133. / 255., 154. / 255., 175. / 255., 188. / 255., 196. / 255., 218. / 255. };
		Double_t blue[NRGBs] = { 208. / 255., 130. / 255., 109. / 255.,  99. / 255., 110. / 255., 122. / 255., 150. / 255., 171. / 255., 218. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Mint
	case 89:
	{
		Double_t red[NRGBs] = { 105. / 255., 106. / 255., 122. / 255., 143. / 255., 159. / 255., 172. / 255., 176. / 255., 181. / 255., 207. / 255. };
		Double_t green[NRGBs] = { 252. / 255., 197. / 255., 194. / 255., 187. / 255., 174. / 255., 162. / 255., 153. / 255., 136. / 255., 125. / 255. };
		Double_t blue[NRGBs] = { 146. / 255., 133. / 255., 144. / 255., 155. / 255., 163. / 255., 167. / 255., 166. / 255., 162. / 255., 174. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Neon
	case 90:
	{
		Double_t red[NRGBs] = { 171. / 255., 141. / 255., 145. / 255., 152. / 255., 154. / 255., 159. / 255., 163. / 255., 158. / 255., 177. / 255. };
		Double_t green[NRGBs] = { 236. / 255., 143. / 255., 100. / 255.,  63. / 255.,  53. / 255.,  55. / 255.,  44. / 255.,  31. / 255.,   6. / 255. };
		Double_t blue[NRGBs] = { 59. / 255.,  48. / 255.,  46. / 255.,  44. / 255.,  42. / 255.,  54. / 255.,  82. / 255., 112. / 255., 179. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Pastel
	case 91:
	{
		Double_t red[NRGBs] = { 180. / 255., 190. / 255., 209. / 255., 223. / 255., 204. / 255., 228. / 255., 205. / 255., 152. / 255.,  91. / 255. };
		Double_t green[NRGBs] = { 93. / 255., 125. / 255., 147. / 255., 172. / 255., 181. / 255., 224. / 255., 233. / 255., 198. / 255., 158. / 255. };
		Double_t blue[NRGBs] = { 236. / 255., 218. / 255., 160. / 255., 133. / 255., 114. / 255., 132. / 255., 162. / 255., 220. / 255., 218. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Pearl
	case 92:
	{
		Double_t red[NRGBs] = { 225. / 255., 183. / 255., 162. / 255., 135. / 255., 115. / 255., 111. / 255., 119. / 255., 145. / 255., 211. / 255. };
		Double_t green[NRGBs] = { 205. / 255., 177. / 255., 166. / 255., 135. / 255., 124. / 255., 117. / 255., 117. / 255., 132. / 255., 172. / 255. };
		Double_t blue[NRGBs] = { 186. / 255., 165. / 255., 155. / 255., 135. / 255., 126. / 255., 130. / 255., 150. / 255., 178. / 255., 226. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Pigeon
	case 93:
	{
		Double_t red[NRGBs] = { 39. / 255., 43. / 255., 59. / 255., 63. / 255., 80. / 255., 116. / 255., 153. / 255., 177. / 255., 223. / 255. };
		Double_t green[NRGBs] = { 39. / 255., 43. / 255., 59. / 255., 74. / 255., 91. / 255., 114. / 255., 139. / 255., 165. / 255., 223. / 255. };
		Double_t blue[NRGBs] = { 39. / 255., 50. / 255., 59. / 255., 70. / 255., 85. / 255., 115. / 255., 151. / 255., 176. / 255., 223. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Plum
	case 94:
	{
		Double_t red[NRGBs] = { 0. / 255., 38. / 255., 60. / 255., 76. / 255., 84. / 255., 89. / 255., 101. / 255., 128. / 255., 204. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 10. / 255., 15. / 255., 23. / 255., 35. / 255., 57. / 255.,  83. / 255., 123. / 255., 199. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 11. / 255., 22. / 255., 40. / 255., 63. / 255., 86. / 255.,  97. / 255.,  94. / 255.,  85. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Red Blue
	case 95:
	{
		Double_t red[NRGBs] = { 94. / 255., 112. / 255., 141. / 255., 165. / 255., 167. / 255., 140. / 255.,  91. / 255.,  49. / 255.,  27. / 255. };
		Double_t green[NRGBs] = { 27. / 255.,  46. / 255.,  88. / 255., 135. / 255., 166. / 255., 161. / 255., 135. / 255.,  97. / 255.,  58. / 255. };
		Double_t blue[NRGBs] = { 42. / 255.,  52. / 255.,  81. / 255., 106. / 255., 139. / 255., 158. / 255., 155. / 255., 137. / 255., 116. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Rose
	case 96:
	{
		Double_t red[NRGBs] = { 30. / 255., 49. / 255., 79. / 255., 117. / 255., 135. / 255., 151. / 255., 146. / 255., 138. / 255., 147. / 255. };
		Double_t green[NRGBs] = { 63. / 255., 60. / 255., 72. / 255.,  90. / 255.,  94. / 255.,  94. / 255.,  68. / 255.,  46. / 255.,  16. / 255. };
		Double_t blue[NRGBs] = { 18. / 255., 28. / 255., 41. / 255.,  56. / 255.,  62. / 255.,  63. / 255.,  50. / 255.,  36. / 255.,  21. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Rust
	case 97:
	{
		Double_t red[NRGBs] = { 0. / 255., 30. / 255., 63. / 255., 101. / 255., 143. / 255., 152. / 255., 169. / 255., 187. / 255., 230. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 14. / 255., 28. / 255.,  42. / 255.,  58. / 255.,  61. / 255.,  67. / 255.,  74. / 255.,  91. / 255. };
		Double_t blue[NRGBs] = { 39. / 255., 26. / 255., 21. / 255.,  18. / 255.,  15. / 255.,  14. / 255.,  14. / 255.,  13. / 255.,  13. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Sandy Terrain
	case 98:
	{
		Double_t red[NRGBs] = { 149. / 255., 140. / 255., 164. / 255., 179. / 255., 182. / 255., 181. / 255., 131. / 255., 87. / 255., 61. / 255. };
		Double_t green[NRGBs] = { 62. / 255.,  70. / 255., 107. / 255., 136. / 255., 144. / 255., 138. / 255., 117. / 255., 87. / 255., 74. / 255. };
		Double_t blue[NRGBs] = { 40. / 255.,  38. / 255.,  45. / 255.,  49. / 255.,  49. / 255.,  49. / 255.,  38. / 255., 32. / 255., 34. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Sienna
	case 99:
	{
		Double_t red[NRGBs] = { 99. / 255., 112. / 255., 148. / 255., 165. / 255., 179. / 255., 182. / 255., 183. / 255., 183. / 255., 208. / 255. };
		Double_t green[NRGBs] = { 39. / 255.,  40. / 255.,  57. / 255.,  79. / 255., 104. / 255., 127. / 255., 148. / 255., 161. / 255., 198. / 255. };
		Double_t blue[NRGBs] = { 15. / 255.,  16. / 255.,  18. / 255.,  33. / 255.,  51. / 255.,  79. / 255., 103. / 255., 129. / 255., 177. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Solar
	case 100:
	{
		Double_t red[NRGBs] = { 99. / 255., 116. / 255., 154. / 255., 174. / 255., 200. / 255., 196. / 255., 201. / 255., 201. / 255., 230. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,   0. / 255.,   8. / 255.,  32. / 255.,  58. / 255.,  83. / 255., 119. / 255., 136. / 255., 173. / 255. };
		Double_t blue[NRGBs] = { 5. / 255.,   6. / 255.,   7. / 255.,   9. / 255.,   9. / 255.,  14. / 255.,  17. / 255.,  19. / 255.,  24. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// South West
	case 101:
	{
		Double_t red[NRGBs] = { 82. / 255., 106. / 255., 126. / 255., 141. / 255., 155. / 255., 163. / 255., 142. / 255., 107. / 255.,  66. / 255. };
		Double_t green[NRGBs] = { 62. / 255.,  44. / 255.,  69. / 255., 107. / 255., 135. / 255., 152. / 255., 149. / 255., 132. / 255., 119. / 255. };
		Double_t blue[NRGBs] = { 39. / 255.,  25. / 255.,  31. / 255.,  60. / 255.,  73. / 255.,  68. / 255.,  49. / 255.,  72. / 255., 188. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Starry Night
	case 102:
	{
		Double_t red[NRGBs] = { 18. / 255., 29. / 255., 44. / 255.,  72. / 255., 116. / 255., 158. / 255., 184. / 255., 208. / 255., 221. / 255. };
		Double_t green[NRGBs] = { 27. / 255., 46. / 255., 71. / 255., 105. / 255., 146. / 255., 177. / 255., 189. / 255., 190. / 255., 183. / 255. };
		Double_t blue[NRGBs] = { 39. / 255., 55. / 255., 80. / 255., 108. / 255., 130. / 255., 133. / 255., 124. / 255., 100. / 255.,  76. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Sunset
	case 103:
	{
		Double_t red[NRGBs] = { 0. / 255., 48. / 255., 119. / 255., 173. / 255., 212. / 255., 224. / 255., 228. / 255., 228. / 255., 245. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 13. / 255.,  30. / 255.,  47. / 255.,  79. / 255., 127. / 255., 167. / 255., 205. / 255., 245. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 68. / 255.,  75. / 255.,  43. / 255.,  16. / 255.,  22. / 255.,  55. / 255., 128. / 255., 245. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Temperature Map
	case 104:
	{
		Double_t red[NRGBs] = { 34. / 255.,  70. / 255., 129. / 255., 187. / 255., 225. / 255., 226. / 255., 216. / 255., 193. / 255., 179. / 255. };
		Double_t green[NRGBs] = { 48. / 255.,  91. / 255., 147. / 255., 194. / 255., 226. / 255., 229. / 255., 196. / 255., 110. / 255.,  12. / 255. };
		Double_t blue[NRGBs] = { 234. / 255., 212. / 255., 216. / 255., 224. / 255., 206. / 255., 110. / 255.,  53. / 255.,  40. / 255.,  29. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Thermometer
	case 105:
	{
		Double_t red[NRGBs] = { 30. / 255.,  55. / 255., 103. / 255., 147. / 255., 174. / 255., 203. / 255., 188. / 255., 151. / 255., 105. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,  65. / 255., 138. / 255., 182. / 255., 187. / 255., 175. / 255., 121. / 255.,  53. / 255.,   9. / 255. };
		Double_t blue[NRGBs] = { 191. / 255., 202. / 255., 212. / 255., 208. / 255., 171. / 255., 140. / 255.,  97. / 255.,  57. / 255.,  30. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Valentine
	case 106:
	{
		Double_t red[NRGBs] = { 112. / 255., 97. / 255., 113. / 255., 125. / 255., 138. / 255., 159. / 255., 178. / 255., 188. / 255., 225. / 255. };
		Double_t green[NRGBs] = { 16. / 255., 17. / 255.,  24. / 255.,  37. / 255.,  56. / 255.,  81. / 255., 110. / 255., 136. / 255., 189. / 255. };
		Double_t blue[NRGBs] = { 38. / 255., 35. / 255.,  46. / 255.,  59. / 255.,  78. / 255., 103. / 255., 130. / 255., 152. / 255., 201. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Visible Spectrum
	case 107:
	{
		Double_t red[NRGBs] = { 18. / 255.,  72. / 255.,   5. / 255.,  23. / 255.,  29. / 255., 201. / 255., 200. / 255., 98. / 255., 29. / 255. };
		Double_t green[NRGBs] = { 0. / 255.,   0. / 255.,  43. / 255., 167. / 255., 211. / 255., 117. / 255.,   0. / 255.,  0. / 255.,  0. / 255. };
		Double_t blue[NRGBs] = { 51. / 255., 203. / 255., 177. / 255.,  26. / 255.,  10. / 255.,   9. / 255.,   8. / 255.,  3. / 255.,  0. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Water Melon
	case 108:
	{
		Double_t red[NRGBs] = { 19. / 255., 42. / 255., 64. / 255.,  88. / 255., 118. / 255., 147. / 255., 175. / 255., 187. / 255., 205. / 255. };
		Double_t green[NRGBs] = { 19. / 255., 55. / 255., 89. / 255., 125. / 255., 154. / 255., 169. / 255., 161. / 255., 129. / 255.,  70. / 255. };
		Double_t blue[NRGBs] = { 19. / 255., 32. / 255., 47. / 255.,  70. / 255., 100. / 255., 128. / 255., 145. / 255., 130. / 255.,  75. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Cool
	case 109:
	{
		Double_t red[NRGBs] = { 33. / 255.,  31. / 255.,  42. / 255.,  68. / 255.,  86. / 255., 111. / 255., 141. / 255., 172. / 255., 227. / 255. };
		Double_t green[NRGBs] = { 255. / 255., 175. / 255., 145. / 255., 106. / 255.,  88. / 255.,  55. / 255.,  15. / 255.,   0. / 255.,   0. / 255. };
		Double_t blue[NRGBs] = { 255. / 255., 205. / 255., 202. / 255., 203. / 255., 208. / 255., 205. / 255., 203. / 255., 206. / 255., 231. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Copper
	case 110:
	{
		Double_t red[NRGBs] = { 0. / 255., 25. / 255., 50. / 255., 79. / 255., 110. / 255., 145. / 255., 181. / 255., 201. / 255., 254. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 16. / 255., 30. / 255., 46. / 255.,  63. / 255.,  82. / 255., 101. / 255., 124. / 255., 179. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 12. / 255., 21. / 255., 29. / 255.,  39. / 255.,  49. / 255.,  61. / 255.,  74. / 255., 103. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Gist Earth
	case 111:
	{
		Double_t red[NRGBs] = { 0. / 255., 13. / 255.,  30. / 255.,  44. / 255.,  72. / 255., 120. / 255., 156. / 255., 200. / 255., 247. / 255. };
		Double_t green[NRGBs] = { 0. / 255., 36. / 255.,  84. / 255., 117. / 255., 141. / 255., 153. / 255., 151. / 255., 158. / 255., 247. / 255. };
		Double_t blue[NRGBs] = { 0. / 255., 94. / 255., 100. / 255.,  82. / 255.,  56. / 255.,  66. / 255.,  76. / 255., 131. / 255., 247. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	break;

	// Viridis
	case 112:
	{
		Double_t red[NRGBs] = { 26. / 255., 51. / 255.,  43. / 255.,  33. / 255.,  28. / 255.,  35. / 255.,  74. / 255., 144. / 255., 246. / 255. };
		Double_t green[NRGBs] = { 9. / 255., 24. / 255.,  55. / 255.,  87. / 255., 118. / 255., 150. / 255., 180. / 255., 200. / 255., 222. / 255. };
		Double_t blue[NRGBs] = { 30. / 255., 96. / 255., 112. / 255., 114. / 255., 112. / 255., 101. / 255.,  72. / 255.,  35. / 255.,   0. / 255. };
		Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, NCont, alpha);
	}
	break;

	Idx *= 1;//Hack by AK to avoid error: variable ‘Idx’ set but not used 

	default:
		//::Error("SetPalette", "Unknown palette number %d", index);
		std::cerr << "SetPalette: Unknown palette number " << index << std::endl;
		return;
	};

	gStyle->SetNumberContours(NCont);
}

