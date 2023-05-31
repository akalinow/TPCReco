#include "ToyIonizationSimulator.h"

fwk::VModule::EResultFlag ToyIonizationSimulator::Init(boost::property_tree::ptree config) {
    auto temp = config.get<double>("Temperature");
    auto pres = config.get<double>("Pressure");
    rangeCalc = std::make_unique<IonRangeCalculator>(gas_mixture_type::CO2,pres*1000,temp);
    pointsPerMm = config.get<double>("PointsPerMm");
    return eSuccess;
}

fwk::VModule::EResultFlag ToyIonizationSimulator::Process(ModuleExchangeSpace &event) {
    auto& simEv = event.simEvt;
    auto origin = simEv.GetTrueVertexPosition();
    for(auto& t : simEv.GetTracks()){
        const auto&  prim = t.GetPrimaryParticle();
        auto direction = prim.GetMomentum().Unit();
        auto length = rangeCalc->getIonRangeMM(prim.GetID(),prim.GetKineticEnergy());
        auto nPoints=std::max((int)(pointsPerMm*length), 10); //minimum 10 points per track
        auto curve = rangeCalc->getIonBraggCurveMeVPerMM(prim.GetID(),prim.GetKineticEnergy(),nPoints);
        for(auto ipoint=0; ipoint<nPoints; ipoint++) { // generate NPOINTS hits along the track
            auto depth = (ipoint + 0.5) * length / nPoints; // mm
            auto hitPosition = origin + direction * depth; // mm
            auto hitDeposit = curve.Eval(depth) * (length / nPoints); // ADC units
            t.InsertHit({hitPosition,hitDeposit});
        }
        t.SortHits();
        t.SetStop(origin+length*direction);
        t.SetTruncatedStop(origin+length*direction);
    }
    return eSuccess;
}

fwk::VModule::EResultFlag ToyIonizationSimulator::Finish() {
    return eSuccess;
}
