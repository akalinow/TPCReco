#include "ToyIonizationSimulator.h"
#include "TPCReco/ConfigManager.h"

fwk::VModule::EResultFlag ToyIonizationSimulator::Init(boost::property_tree::ptree config) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    // pointsPerMm = config.get<double>("PointsPerMm"); // without MATH expressions
    // auto temp = config.get<double>("Temperature"); // without MATH expressions
    // auto pres = config.get<double>("Pressure"); // without MATH expressions
    pointsPerMm = ConfigManager::getScalar<double>(config, "PointsPerMm"); // [1/mm]
    auto temp = ConfigManager::getScalar<double>(config, "Temperature"); // [K]
    auto pres = ConfigManager::getScalar<double>(config, "Pressure"); // [mbar]
    rangeCalc = std::make_unique<IonRangeCalculator>(gas_mixture_type::CO2, pres*1000, temp);
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
        for(auto ipoint=0; ipoint<=nPoints; ipoint++) { // generate NPOINTS+1 hits along the track
            auto depth = ipoint * length / nPoints; // mm
            auto hitPosition = origin + direction * depth; // mm
            auto hitDeposit = curve.Eval(depth) * (length / nPoints); // ADC units
            t.InsertHit({hitPosition,hitDeposit});
        }
	/*
        t.SortHits();
        t.SetStop(origin+length*direction);
        t.SetTruncatedStop(origin+length*direction);
	*/
        //first we sort hits, so that they start at the vertex and continue outward in the vector
        t.SortHits();
        //Now we set the track stop position (hit furthest from the start):
        t.RecalculateStopPosition();

        //set truncated positions to be tha same as the real ones - no truncation has been performed yet
        t.SetTruncatedStart(t.GetStart());
        t.SetTruncatedStop(t.GetStop());
    }
    return eSuccess;
}

fwk::VModule::EResultFlag ToyIonizationSimulator::Finish() {
    return eSuccess;
}
