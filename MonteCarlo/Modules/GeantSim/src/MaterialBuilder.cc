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

MaterialBuilder *MaterialBuilder::instance = nullptr;

MaterialBuilder *MaterialBuilder::GetInstance() {
    if (instance == nullptr)
        instance = new MaterialBuilder();
    return instance;
}

MaterialBuilder::MaterialBuilder() {
    nist_manager = G4NistManager::Instance();
    BuildMaterials();
}

void MaterialBuilder::BuildMaterials() {
    G4String name, symbol;
    G4double density;

    G4double temperature = 293. * kelvin, pressure;
    //Define elements:
    auto H = new G4Element("Hydrogen", "H", 1., 1.01 * g / mole);
    auto He = new G4Element("Helium", "He", 2., 4.0 * g / mole);

    auto N = new G4Element("Nitrogen", "N", 7., 14.01 * g / mole);
    auto O = new G4Element("Oxygen", "O", 8., 16.00 * g / mole);
    auto Ar = new G4Element("Argon", "Ar", 18., 39.948 * g / mole);
    auto C = new G4Element("Carbon", "C", 6., 12.01 * g / mole);
    auto Fe = new G4Element("Iron", "Fe", 26., 55.85 * g / mole);
    auto Cr = new G4Element("Chromium", "Cr", 24., 51.9961 * g / mole);
    auto Mn = new G4Element("Manganese", "Mn", 12., 54.938049 * g / mole);
    auto Ni = new G4Element("Nickel", "Ni", 28., 58.6934 * g / mole);
    auto Si = new G4Element("Silicon", "Si", 14., 28.09 * g / mole);

    //Air:
    density = 1.290 * mg / cm3;
    auto Air = new G4Material("Air", density, 2);
    Air->AddElement(N, 0.7);
    Air->AddElement(O, 0.3);
    materials["air"] = Air;

    //aluminium:
    auto alu = new G4Material("Aluminium", 13., 26.98 * g / mole, 2.7 * g / cm3);
    materials["aluminium"] = alu;
    //Kapton:
    density = 1.42 * g / cm3;
    auto Kapton = new G4Material("Kapton", density, 4);
    Kapton->AddElement(H, 0.0273);
    Kapton->AddElement(C, 0.7213);
    Kapton->AddElement(N, 0.0765);
    Kapton->AddElement(O, 0.1749);
    materials["kapton"] = Kapton;

    //StainlessSteel
    density = 8.06 * g / cm3;
    auto StainlessSteel = new G4Material("StainlessSteel", density, 6);
    StainlessSteel->AddElement(C, 0.001);
    StainlessSteel->AddElement(Si, 0.007);
    StainlessSteel->AddElement(Cr, 0.18);
    StainlessSteel->AddElement(Mn, 0.01);
    StainlessSteel->AddElement(Fe, 0.712);
    StainlessSteel->AddElement(Ni, 0.09);
    materials["stainless"] = StainlessSteel;

    CentralConfig *config = CentralConfig::GetInstance();
    auto p_he = config->Get<float>("gas_mixture.he");
    auto p_co2 = config->Get<float>("gas_mixture.co2");


    //G4double CO2_density = 1.805 * mg / cm3;
    G4double CO2_density = 1.787 * mg / cm3;
    auto CO2 = new G4Material("Carbonic gas", CO2_density, 2,
                              kStateGas, 293. * kelvin, p_co2 * bar);
    CO2->AddElement(C, 1);
    CO2->AddElement(O, 2);
    materials["co2"] = CO2;

    G4double He_density = 0.1645 * mg / cm3;
    auto He_gas = new G4Material("Helium gas", He_density, 1,
                                 kStateGas, 293. * kelvin, p_he * bar);
    He_gas->AddElement(He, 1);
    materials["he"] = He_gas;

    auto mixture = new G4Material("mixture", p_co2 * CO2_density+ p_he * He_density,
                                  2,
                                  kStateGas, temperature, (p_co2 + p_co2) * bar);
    mixture->AddMaterial(He_gas, p_he / (p_he + p_co2));
    mixture->AddMaterial(CO2, p_co2 / (p_he + p_co2));
    materials["mixture"] = mixture;

    // Vacuum
    density = 1.e-5 * g / cm3;
    pressure = 2.e-2 * bar;
    temperature = STP_Temperature;         //from PhysicalConstants.h
    auto vacuum = new G4Material("vacuum", density, 1,
                                 kStateGas, temperature, pressure);
    vacuum->AddMaterial(Air, 1.);
    materials["vacuum"] = vacuum;

    // Laboratory vacuum: Dry air (average composition)
    density = 1.7836 * mg / cm3;       // STP
    auto Argon = new G4Material("Argon", density, 1);
    Argon->AddElement(Ar, 1);

    density = 1.25053 * mg / cm3;       // STP
    auto Nitrogen = new G4Material("N2", density, 1);
    Nitrogen->AddElement(N, 2);

    density = 1.4289 * mg / cm3;       // STP
    auto Oxygen = new G4Material("O2", density, 1);
    Oxygen->AddElement(O, 2);


    density = 1.2928 * mg / cm3;       // STP
    density *= 1.0e-8;              // pumped vacuum

    temperature = STP_Temperature;
    pressure = 1.0e-8 * STP_Pressure;
    auto LaboratoryVacuum = new G4Material("LaboratoryVacuum",
                                           density, 3,
                                           kStateGas, temperature, pressure);
    LaboratoryVacuum->AddMaterial(Nitrogen, 0.7557);
    LaboratoryVacuum->AddMaterial(Oxygen, 0.2315);
    LaboratoryVacuum->AddMaterial(Argon, 0.0128);
    materials["laboratoryvacuum"] = LaboratoryVacuum;

    auto SiO2 =
            new G4Material("quartz", 2.200 * g / cm3, 2);
    SiO2->AddElement(Si, 1);
    SiO2->AddElement(O, 2);

    density = 1.2 * g / cm3;
    auto Epoxy = new G4Material("Epoxy", density, 2);
    Epoxy->AddElement(H, 2);
    Epoxy->AddElement(C, 2);

    density = 1.86 * g / cm3;
    auto FR4 = new G4Material("FR4", density, 2);
    FR4->AddMaterial(SiO2, 0.528);
    FR4->AddMaterial(Epoxy, 0.472);

    materials["FR4"] = FR4;
    materials["copper"] = new G4Material("Copper", 29., 63.546 * g / mole, 8.96 * g / cm3);

    auto Peek = new G4Material("Peek", 1.31 * g / cm3, 3);
    Peek->AddMaterial(nist_manager->FindOrBuildMaterial("G4_C"), 76 * perCent);
    Peek->AddMaterial(nist_manager->FindOrBuildMaterial("G4_H"), 8 * perCent);
    Peek->AddMaterial(nist_manager->FindOrBuildMaterial("G4_O"), 16 * perCent);
    materials["peek"] = Peek;


}

G4Material *MaterialBuilder::GetMaterial(const G4String &material_name) {
    if (!materials.count(material_name)) {
        std::string desc = "Requested material '" + material_name +
                           "' does not exist. Check config for typos and make sure that the material is defined properly in MaterialBuilder::BuildMaterials";
        G4Exception("MaterialBuilder::GetMaterial", "No such material",
                    FatalException, desc.c_str()
        );
    }

    return materials[material_name];
}