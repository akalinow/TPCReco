#include "TPCReco/EventGenerator.h"
#include "Math/EulerAngles.h"

EventGenerator::EventGenerator(const boost::property_tree::ptree &configNode) : setup{configNode} {
    Init();
}

EventGenerator::EventGenerator(const boost::filesystem::path &configFilePath) : setup{configFilePath} {

    Init();
}

void EventGenerator::Init() {
    setup.BuildReactionLibrary(lib);
    xyProv = setup.BuildXYProvider();
    zProv = setup.BuildZProvider();
    eProv = setup.BuildEProvider();
    auto g = setup.ReadBeamGeometry();
    auto eaNom = ROOT::Math::EulerAngles(g.phiNom, g.thetaNom, g.psiNom);
    auto eaAct = ROOT::Math::EulerAngles(g.phiAct, g.thetaAct, g.psiAct);
    //Inverse here, because angles in config file describe DET -> BEAM transformation
    beamToDetRotation = ROOT::Math::Rotation3D(eaAct * eaNom).Inverse();
    beamPosition = g.beamPos;
}

ROOT::Math::XYZPoint EventGenerator::GenerateVertexPosition() {
    double x,y,z;
    z=zProv->GetZ();
    std::tie(x,y)=xyProv->GetXY();
    return beamToDetRotation*ROOT::Math::XYZPoint(x,y,z)+beamPosition;
}

SimEvent EventGenerator::GenerateEvent() {
    auto pos=GenerateVertexPosition();
    auto vtxPos=TVector3(pos.X(),pos.Y(),pos.Z());
    auto gammaMom=eProv->GetEnergy();
    reaction_type rType;
    PrimaryParticles primaries;
    std::tie(primaries, rType)=lib.Generate(gammaMom,beamToDetRotation);
    SimTracks tracks;
    for(const auto &p: primaries)
        tracks.emplace_back(p,vtxPos);
    return {tracks,vtxPos,rType};
}
