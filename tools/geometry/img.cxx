#include <TFile.h>
#include <TCanvas.h>
#include <TH2Poly.h>
int img(){
	TFile f("test.root","READ");
		
	TCanvas* c=new TCanvas("c");	
	//c->SetCanvasSize(2000,1000);
	TH2Poly *a= (TH2Poly*) f. Get ("a");
	TH2Poly *b= (TH2Poly*) f. Get ("b");
	gStyle->SetPalette(kDarkRainBow);
	gStyle->SetHistLineWidth(0);
	b->UseCurrentStyle();
	a->UseCurrentStyle();
		a->SetTitle("");
		std::cout<<a->GetLineWidth();
	b->SetTitle("");
	a->SetStats(0);
	b->SetStats(0);
	a->SetContour(0);
	//a->SetContourLevel(0);
	//b->SetLineWidth(0.001);
	b->Draw("col a  " );
	a->Draw("same SPEC");
	

c->Print("img.pdf");
return 0;
}