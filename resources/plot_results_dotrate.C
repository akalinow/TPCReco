int plot_results_dotrate(const char *input_root="results.root", const char *output_prefix="plots") {

  gStyle->SetTitleOffset(1.25, "xy");
  gStyle->SetTitleOffset(0.95, "z");
  gROOT->ForceStyle();
  const Float_t statX=gStyle->GetStatX();
  const Float_t statY=gStyle->GetStatY();
  const Float_t marginR=gStyle->GetPadRightMargin();

  TFile f(input_root, "OLD");
  if (f.IsZombie()) {
    cout << "Error opening file" << endl;
    exit(-1);
  }
  f.cd();
  const std::string out=(std::string)(output_prefix)+".pdf";

  // Check TH1D/F, TH2D/F, TProfile/2D
  TIter next(f.GetListOfKeys());
  TKey *key;
  long counter=0;
  TCanvas *c=NULL;
  while ((key=(TKey*)next())) {
    printf("key: %s points to an object of class: %s\n",
	   key->GetName(),
	   key->GetClassName());
    const TString cname(key->GetClassName());
    const TString hname(key->GetName());
    if(cname=="TH1D" || cname=="TH1F" ||
       cname=="TH2D" || cname=="TH2F" ||
       cname=="TProfile" || cname=="TProfile2D") counter++;
    else {
      printf("Class %s is out of the requested list\n", key->GetClassName());
      continue;
    }

    // open output PDF file
    if(counter==1) {
      c=new TCanvas("c","c",1200+200, 1200*20/33+100); 
      c->cd();
      c->UseCurrentStyle();
      c->Print((out+"[").c_str());
    }

    // draw histograms
    if(cname=="TH1D") { 
      TH1D *h=(TH1D*) f.Get(hname);
      if (h) {
	// Change axis labels for rate plots
	if(hname.BeginsWith("h_timediff")) {
	  h->SetXTitle("Time difference [s]");
	  h->SetYTitle("Events/bin");
	}
	// Change axis labels for dot position plots
	if(hname.BeginsWith("h_x_")) {
	  h->SetXTitle("Vertex X [mm]");
	  h->SetYTitle("Events/bin");
	}
	if(hname.BeginsWith("h_y_")) {
	  h->SetXTitle("Vertex Y [mm]");
	  h->SetYTitle("Events/bin");
	}
	if(hname.BeginsWith("h_z_")) {
	  h->SetXTitle("Vertex Z [mm]");
	  h->SetYTitle("Events/bin");
	}
	gStyle->SetStatX(statX);
	gStyle->SetStatY(statY);
	gStyle->SetPadRightMargin(marginR);
	gStyle->SetOptStat(1111); // name+entries+mean+rms
	c->UseCurrentStyle();
	h->SetLineColor(kBlue);
	h->SetFillColor(kBlue);
	h->Draw(""); 
	c->Print(out.c_str()); 
      }
    } 
    //    if(cname=="TH1F") { 
    //      TH1F *h=(TH1F*) f.Get(hname);
    //      if (h) {
    //	gStyle->SetOptStat(1111); // name+entries+mean+rms
    //	h->Draw(); 
    //	c->Print(out.c_str()); 
    //      }
    //    }
    if(cname=="TH2D") { 
      TH2D *h=(TH2D*) f.Get(hname);
      if (h) {
	if(hname.BeginsWith("h_xy_")) {
	  h->SetXTitle("Vertex X [mm]");
	  h->SetYTitle("Vertex Y [mm]");
	  h->SetZTitle("Events/bin");
	}
	gStyle->SetStatX(statX);
	gStyle->SetStatY(0.995);
	gStyle->SetOptStat(11); // name+entries
	c->UseCurrentStyle();
	h->Draw("COLZ"); 
	gPad->SetRightMargin(0.125);
	c->Print(out.c_str()); 
	TH2D *hnew=(TH2D*)(h->Rebin2D(8, 8, ((std::string)(h->GetName())+"_rebin8").c_str() )); 
	if (hnew) {
	  hnew->Draw("COLZ"); 
	  c->Print(out.c_str()); 
	}
      }
    }
    //    if(cname=="TH2F") { 
    //      TH2F *h=(TH2F*) f.Get(hname); 
    //      if (h) {
    //	gStyle->SetOptStat(11); // name+entries
    //	h->Draw("COLZ"); 
    //	c->Print(out.c_str()); 
    //	TH2F *hnew=(TH2F*)(h->Rebin2D(8, 8, ((std::string)(h->GetName())+"_rebin8").c_str() )); 
    //	if (hnew) {
    //	  hnew->Draw("COLZ"); 
    //	  c->Print(out.c_str()); 
    //	}
    //      }
    //    }
    if(cname=="TProfile") { 
      TProfile *h=(TProfile*) f.Get(hname); 
      if (h) {
	if(hname.BeginsWith("prof_deltaz_x_")) {
	  h->SetXTitle("Vertex X [mm]");
	  h->SetYTitle("Average #DeltaZ [mm]");
	}
	if(hname.BeginsWith("prof_deltaz_y_")) {
	  h->SetXTitle("Vertex Y [mm]");
	  h->SetYTitle("Average #DeltaZ [mm]");
	}
	gStyle->SetStatX(statX);
	gStyle->SetStatY(statY);
	gStyle->SetPadRightMargin(marginR);
	gStyle->SetOptStat(1111); // name+entries+mean+rms
	c->UseCurrentStyle();
	h->Draw(); 
	c->Print(out.c_str()); 
	TProfile *hnew=(TProfile*)(h->RebinX(8, ((std::string)(h->GetName())+"_rebin8").c_str() )); 
	if (hnew) {
	  hnew->Draw(); 
	  c->Print(out.c_str()); 
	}
      }
    }
    if(cname=="TProfile2D") { 
      TProfile2D *h=(TProfile2D*) f.Get(hname);
      if (h) {
	if(hname.BeginsWith("prof_deltaz_xy_")) {
	  h->SetXTitle("Vertex X [mm]");
	  h->SetYTitle("Vertex Y [mm]");
	  h->SetZTitle("Average #DeltaZ [mm]");
	}
	gStyle->SetStatX(statX);
	gStyle->SetStatY(0.995);
	gStyle->SetOptStat(11); // name+entries
	c->UseCurrentStyle();
	h->Draw("COLZ"); 
	gPad->SetRightMargin(0.125);
	c->Print(out.c_str()); 
	TProfile2D *hnew=(TProfile2D*)(h->Rebin2D(8, 8, ((std::string)(h->GetName())+"_rebin8").c_str() )); 
	if (hnew) {
	  hnew->Draw("COLZ"); 
	  c->Print(out.c_str()); 
	}
      }
    }

  } // end of while(...)

  // close output PDF file
  if(counter) {
    c->Print((out+"]").c_str());
  }

  f.Close();
  return 0;
}
