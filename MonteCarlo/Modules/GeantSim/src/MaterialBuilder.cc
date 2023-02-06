/**
 * @file MaterialBuilder.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of MaterialBuilder  class
 */

#include "MaterialBuilder.hh"
#include "G4NistManager.hh"
#include "globals.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "CentralConfig.hh"

MaterialBuilder* MaterialBuilder::instance=nullptr;

MaterialBuilder* MaterialBuilder::GetInstance()
{
	if(instance==nullptr)
		instance=new MaterialBuilder();
	return instance;
}

MaterialBuilder::MaterialBuilder()
{
	nist_manager=G4NistManager::Instance();
	BuildMaterials();
}

void MaterialBuilder::BuildMaterials()
{
	G4String name, symbol;             
	G4double density;            

	G4int ncomponents, natoms;
	G4double fractionmass;
	G4double temperature, pressure;
	//Define elements:
	G4Element*   H  = new G4Element ("Hydrogen", "H", 1. ,  1.01*g/mole);
	G4Element*   He  = new G4Element ("Helium", "He", 2. ,  4.0*g/mole);

	G4Element*   N  = new G4Element ("Nitrogen", "N", 7., 14.01*g/mole);
	G4Element*   O  = new G4Element ("Oxygen"  , "O", 8. , 16.00*g/mole);
	G4Element*   Ar = new G4Element ("Argon" , "Ar", 18., 39.948*g/mole );
	G4Element*   C = new G4Element ("Carbon" , "C", 6., 12.01*g/mole );
	G4Element* Fe = new G4Element("Iron","Fe", 26., 55.85*g/mole);
	G4Element* Cr = new G4Element("Chromium", "Cr", 24., 51.9961*g/mole);
	G4Element* Mn = new G4Element("Manganese", "Mn", 12., 54.938049*g/mole);
	G4Element* Ni = new G4Element("Nickel", "Ni", 28.,  58.6934*g/mole);
	G4Element*   Si = new G4Element("Silicon", "Si", 14., 28.09*g/mole);

	//Air:
	density = 1.290*mg/cm3;
	G4Material* Air = new G4Material(name="Air"  , density, ncomponents=2);
	Air->AddElement(N, fractionmass=0.7);
	Air->AddElement(O, fractionmass=0.3);
	materials["air"]=Air;

	//aluminium:
	G4Material* alu=new G4Material("Aluminium",13., 26.98*g/mole,2.7*g/cm3);
	materials["aluminium"]=alu;
	//Kapton:
	density = 1.42*g/cm3;
	G4Material* Kapton= new G4Material(name="Kapton",density, ncomponents=4);
	Kapton->AddElement(H, fractionmass = 0.0273);
	Kapton->AddElement(C, fractionmass = 0.7213);
	Kapton->AddElement(N, fractionmass = 0.0765);
	Kapton->AddElement(O, fractionmass = 0.1749);
	materials["kapton"]=Kapton;

  	 //StainlessSteel
	density= 8.06*g/cm3;
	G4Material* StainlessSteel = new G4Material(name="StainlessSteel", density, ncomponents=6);
	StainlessSteel->AddElement(C, fractionmass=0.001);
	StainlessSteel->AddElement(Si, fractionmass=0.007);
	StainlessSteel->AddElement(Cr, fractionmass=0.18);
	StainlessSteel->AddElement(Mn, fractionmass=0.01);
	StainlessSteel->AddElement(Fe, fractionmass=0.712);
	StainlessSteel->AddElement(Ni, fractionmass=0.09);
	materials["stainless"]=StainlessSteel;

	CentralConfig* config=CentralConfig::GetInstance();
	float p_he=config->GetD("gas_mixture","he");
	float p_co2=config->GetD("gas_mixture","co2");

	G4Material* CO2 = new G4Material(name="Carbonic gas", density = 1.805*mg/cm3, ncomponents=2,
		kStateGas,temperature=293.*kelvin, pressure    = p_co2*bar);
	CO2->AddElement(C, natoms=1);
	CO2->AddElement(O, natoms=2);
	materials["co2"]=CO2;

	G4Material* He_gas = new G4Material(name="Helium gas", density = 0.1645*mg/cm3, ncomponents=1,
		kStateGas,temperature=293.*kelvin, pressure  = p_he*bar);
	He_gas->AddElement(He, natoms=1);
	materials["he"]=He_gas;

	G4Material* mixture = new G4Material(name="mixture", density=p_co2*1.805*mg/cm3+p_he*0.1645*mg/cm3, ncomponents=2,
 		kStateGas,temperature,(p_co2+p_co2)*bar);
	mixture->AddMaterial(He_gas, fractionmass=p_he/(p_he+p_co2));
	mixture->AddMaterial(CO2, fractionmass=p_co2/(p_he+p_co2));
	materials["mixture"]=mixture;

    // Vacuum
	density     = 1.e-5*g/cm3;
	pressure    = 2.e-2*bar;
 	temperature = STP_Temperature;         //from PhysicalConstants.h
 	G4Material* vacuum = new G4Material(name="vacuum", density, ncomponents=1,
 		kStateGas,temperature,pressure);
 	vacuum->AddMaterial(Air, fractionmass=1.);
 	materials["vacuum"]=vacuum;

  	// Laboratory vacuum: Dry air (average composition)
	density = 1.7836*mg/cm3 ;       // STP
	G4Material* Argon = new G4Material(name="Argon", density, ncomponents=1);
	Argon->AddElement(Ar, 1);
	  
	density = 1.25053*mg/cm3 ;       // STP
	G4Material* Nitrogen = new G4Material(name="N2", density, ncomponents=1);
	Nitrogen->AddElement(N, 2);
	
	density = 1.4289*mg/cm3 ;       // STP
	G4Material* Oxygen = new G4Material(name="O2", density, ncomponents=1);
	Oxygen->AddElement(O, 2);
	  
	  
	density  = 1.2928*mg/cm3 ;       // STP
	density *= 1.0e-8 ;              // pumped vacuum
	
	temperature = STP_Temperature;
	pressure = 1.0e-8*STP_Pressure;
	G4Material* LaboratoryVacuum = new G4Material(name="LaboratoryVacuum",
		density,ncomponents=3,
		kStateGas,temperature,pressure);
	LaboratoryVacuum->AddMaterial( Nitrogen, fractionmass = 0.7557 ) ;
	LaboratoryVacuum->AddMaterial( Oxygen,   fractionmass = 0.2315 ) ;
	LaboratoryVacuum->AddMaterial( Argon,    fractionmass = 0.0128 ) ;
	materials["laboratoryvacuum"]=LaboratoryVacuum;

	G4Material* SiO2 = 
	new G4Material("quartz",density= 2.200*g/cm3, ncomponents=2);
	SiO2->AddElement(Si, natoms=1);
	SiO2->AddElement(O , natoms=2);

	density = 1.2*g/cm3;
	G4Material* Epoxy = new G4Material("Epoxy" , density, ncomponents=2);
	Epoxy->AddElement(H, natoms=2);
	Epoxy->AddElement(C, natoms=2);

	density = 1.86*g/cm3;
	G4Material* FR4 = new G4Material("FR4"  , density, ncomponents=2);
	FR4->AddMaterial(SiO2, fractionmass=0.528);
	FR4->AddMaterial(Epoxy, fractionmass=0.472);

	materials["FR4"]=FR4;
	G4double z,a;
	materials["copper"]=new G4Material("Copper", z= 29., a= 63.546*g/mole, density= 8.96*g/cm3);

	G4Material *Peek = new G4Material("Peek",   density = 1.31*g/cm3, 3);
  	Peek->AddMaterial(nist_manager->FindOrBuildMaterial("G4_C"), 76*perCent);
  	Peek->AddMaterial(nist_manager->FindOrBuildMaterial("G4_H"),  8*perCent);
  	Peek->AddMaterial(nist_manager->FindOrBuildMaterial("G4_O"), 16*perCent);
  	materials["peek"] = Peek;


	}

	G4Material* MaterialBuilder::GetMaterial(G4String material_name)
	{
		if(!materials.count(material_name))
			G4Exception("MaterialBuilder::GetMaterial", "No such material",
				FatalException, "Requested material does not exist. Check config for typos and make sure that the material is defined properly in MaterialBuilder::BuildMaterials");
		return materials[material_name];
	}