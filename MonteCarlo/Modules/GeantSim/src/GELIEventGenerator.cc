/**
 * @file GELIEventGenerator.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIEventGenerator class
 */

#ifdef USE_EVENT_GENERATOR
#include "GELIEventGenerator.hh"
#include "CentralConfig.hh"

GELIEventGenerator::GELIEventGenerator()
{
	CentralConfig* config=CentralConfig::GetInstance();
	std::string configFileName=config->Get("primary_generator","EventGenerator","config_file");
	configFile = new ConfigFile(configFileName);
    simulationSetupFactory = new SimulationSetupFactory(configFile);
    setup = simulationSetupFactory->CreateSetup();
    photodisintegration = new Photodisintegration();
    targetMass = setup->GetTargetMass();
	firstProdMassMass = setup->GetFirstProdMass();
	secProdMassMass = setup->GetSecProdMass();
	firstProdMassNumber=configFile->GetFirstProdMassNumber();
	firstProdAtomicNumber=configFile->GetFirstProdAtomicNumber();
	secondProdMassNumber=configFile->GetSecondProdMassNumber();
	secondProdAtomicNumber=configFile->GetSecondProdAtomicNumber();
}

ReactionResult GELIEventGenerator::GetReactionProducts()
{

	double gammaEn = setup->GetNextGammaEn();
	double thetaAngle = setup->GetNextTheta();
	double phiAngle = setup->GetNextPhi();
	return photodisintegration->GetReactionProducts(gammaEn, thetaAngle, phiAngle, targetMass, firstProdMassMass, secProdMassMass);
}

void GELIEventGenerator::FillVectors(vec &theta1, vec &theta2, vec &phi1, vec &phi2, vec &E1, vec &E2)
{
	ReactionResult r=GetReactionProducts();
	theta1.push_back(r.thetaLABfirst);
	theta2.push_back(r.thetaLABsec);
	phi1.push_back(r.phiLABfirst);
	phi2.push_back(r.phiLABsec);
	E1.push_back(r.energyLABfirst);
	E2.push_back(r.energyLABsec);

}
#endif