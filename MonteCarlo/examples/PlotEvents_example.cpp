#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TView.h"
#include "TPolyLine3D.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/Track3D.h"

#include <boost/program_options.hpp>

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

    boost::program_options::options_description cmdLineOptDesc("Allowed command line options");

    cmdLineOptDesc.add_options()
            ("help", "produce help message")
            ("geometryFile",  boost::program_options::value<std::string>(), "string - path to TPC geometry file")
            ("dataFile",  boost::program_options::value<std::string>(), "string - path to ROOT data file")
            ("maxNevents", boost::program_options::value<Long64_t>(), "int - number of events to process, default: all");

    boost::program_options::variables_map varMap;

    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
        if (varMap.count("help") || varMap.size()==1) {
            std::cout << std::endl
                      << "PlotEvents [options]" << std::endl << std::endl;
            std::cout << cmdLineOptDesc << std::endl;
            exit(1);
        }
        boost::program_options::notify(varMap);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        std::cout << cmdLineOptDesc << std::endl;
        exit(1);
    }

    return varMap;
}

void wirePlotDriftCage3D(std::unique_ptr<TCanvas> &outputCanvas, const std::shared_ptr<GeometryTPC>& aGeometry, bool simul_ext_trg_flag) {

    if(!outputCanvas) return;
    outputCanvas->cd();

    // make wire plot of drift cage in 3D
    TView *view=TView::CreateView(1);
    double xmin, xmax, ymin, ymax, zmin, zmax;
    std::tie(xmin, xmax, ymin, ymax, zmin, zmax)=aGeometry->rangeXYZ();

    if(simul_ext_trg_flag) {
        auto err=false;
        auto get_zmin=aGeometry->Timecell2pos(0, err);
        zmax=get_zmin+(zmax-zmin);
        zmin=get_zmin;
    }

    auto view_span=0.8*std::max(std::max(xmax-xmin, ymax-ymin), zmax-zmin);
    view->SetRange(0.5*(xmax+xmin)-0.5*view_span, 0.5*(ymax+ymin)-0.5*view_span, 0.5*(zmax+zmin)-0.5*view_span,
                   0.5*(xmax+xmin)+0.5*view_span, 0.5*(ymax+ymin)+0.5*view_span, 0.5*(zmax+zmin)+0.5*view_span);
    // plot active volume's faces
    TGraph gr=aGeometry->GetActiveAreaConvexHull();
    TPolyLine3D l(5*(gr.GetN()-1));
    for(auto iedge=0; iedge<gr.GetN()-1; iedge++) {
        l.SetPoint(iedge*5+0, gr.GetX()[iedge], gr.GetY()[iedge], zmin);
        l.SetPoint(iedge*5+1, gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmin);
        l.SetPoint(iedge*5+2, gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmax);
        l.SetPoint(iedge*5+3, gr.GetX()[iedge], gr.GetY()[iedge], zmax);
        l.SetPoint(iedge*5+4, gr.GetX()[iedge], gr.GetY()[iedge], zmin);
    }
    l.SetLineColor(kBlue);
    l.DrawClone();
    outputCanvas->Update();
    outputCanvas->Modified();
}

void wirePlotTrack3D(std::unique_ptr<TCanvas> &outputCanvas, Track3D *aTrack, Color_t color=kRed) {

    if(!outputCanvas || !aTrack) return;
    outputCanvas->cd();

    // make wire plot of track collection in 3D
    for(auto t : aTrack->getSegments()) {
        TPolyLine3D l(2);
        l.SetPoint(0, t.getStart().X(), t.getStart().Y(), t.getStart().Z());
        l.SetPoint(1, t.getEnd().X(), t.getEnd().Y(), t.getEnd().Z());
        l.SetLineColor(color);
        l.SetLineWidth(2);
        if(t.getLength()>1.0) l.DrawClone(); // skip tracks below 1mm
    }
}

int main(int argc, char** argv){
    auto varMap = parseCmdLineArgs(argc, argv);
    if(!varMap.count("geometryFile")){
        std::cerr<<"geometryFile not provided, quitting!"<<std::endl;
        return -1;
    }




    if(!varMap.count("dataFile")){
        std::cerr<<"dataFile not provided, quitting!"<<std::endl;
        return -1;
    }

    auto geometry = std::make_shared<GeometryTPC>(varMap["geometryFile"].as<std::string>().c_str());
    auto dataFileName = varMap["dataFile"].as<std::string>();

    std::unique_ptr<TFile> dataFile{TFile::Open(dataFileName.c_str())};
    if(!dataFile){
        std::cerr<<"Error opening ROOT file: "<<dataFileName<<", quitting!"<<std::endl;
        return -1;
    }

    TTree* trackTree;
    dataFile->GetObject("TPCRecoData",trackTree);

    auto aTrack = new Track3D();
    TBranch *aBranch  = trackTree->GetBranch("RecoEvent");
    if(!aBranch) {
        std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
        return -1;
    }
    aBranch->SetAddress(&aTrack);

    auto c = std::make_unique<TCanvas>("c","all events",1000,1000);

    auto nEventsToDraw = trackTree->GetEntries();

    if(varMap.count("maxNevents")){
        nEventsToDraw = std::min(nEventsToDraw, varMap["maxNevents"].as<Long64_t>());
    }

    //draw detector:
    wirePlotDriftCage3D(c,geometry,true);

    for(Long64_t i=0;i<nEventsToDraw;i++){
        trackTree->GetEntry(i);
        wirePlotTrack3D(c,aTrack);
    }

    c->Print("Generated_wirePlotTrack3D.pdf");
    c->Print("Generated_wirePlotTrack3D.C");

}