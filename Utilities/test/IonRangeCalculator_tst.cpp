#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/CommonDefinitions.h"

using ::testing::DoubleNear;
using ::testing::Gt;
using ::testing::Lt;

namespace fs = boost::filesystem;
class IonRangeCalculatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test data files
        directory = (fs::temp_directory_path() / fs::unique_path()).string() +
        fs::path::preferred_separator;
        fs::create_directories(directory);
        
        // Create minimal test data files
        createTestDataFiles();
        
        // Initialize calculator with test data
        calculator = std::make_unique<IonRangeCalculator>(
            directory.string() + "/", 
            gas_mixture_type::CO2, 
            250.0,  // pressure in mbar
            293.15, // temperature in K (20°C)
            false   // debug flag
        );
    }
    
    void TearDown() override {
        calculator.reset();
        fs::remove_all(directory);
    }
    
    void createTestDataFiles() {
        // Create test range data file for proton
        createRangeDataFile("range_corr_thr_1keV_proton_2MeV_CO2_250mbar.dat",
            "0.001 0.01\n"
            "0.1 0.5\n"
            "0.5 2.0\n" 
            "1.0 5.0\n"
            "2.0 15.0\n");
            
        // Create test range data file for alpha
        createRangeDataFile("range_corr_thr_1keV_alpha_10MeV_CO2_250mbar.dat",
            "0.1 0.1\n"
            "1.0 0.8\n"
            "5.0 3.5\n"
            "10.0 8.0\n");
            
        // Create test range data file for carbon-12
        createRangeDataFile("range_corr_thr_1keV_12C_5MeV_CO2_250mbar.dat",
            "0.1 0.05\n"
            "1.0 0.4\n"
            "2.5 1.2\n"
            "5.0 3.0\n");
            
        // Create test Bragg curve data files
        createBraggDataFile("dEdx_corr_proton_2MeV_CO2_250mbar.dat",
            "0.0 500.0\n"
            "2.0 600.0\n"
            "8.0 800.0\n"
            "14.0 1200.0\n"
            "15.0 200.0\n");
            
        createBraggDataFile("dEdx_corr_alpha_10MeV_CO2_250mbar.dat",
            "0.0 1000.0\n"
            "2.0 1200.0\n"
            "6.0 1800.0\n"
            "7.5 2500.0\n"
            "8.0 400.0\n");
            
        createBraggDataFile("dEdx_corr_12C_5MeV_CO2_250mbar.dat",
            "0.0 2000.0\n"
            "1.0 2200.0\n"
            "2.5 3000.0\n"
            "2.9 4000.0\n"
            "3.0 800.0\n");
    }
    
    void createRangeDataFile(const std::string& filename, const std::string& content) {
        std::ofstream file((directory / filename).string());
        file << content;
        file.close();
    }
    
    void createBraggDataFile(const std::string& filename, const std::string& content) {
        std::ofstream file((directory / filename).string());
        file << content;
        file.close();
    }
    
    fs::path directory;
    std::unique_ptr<IonRangeCalculator> calculator;
};

// Constructor Tests
TEST_F(IonRangeCalculatorTest, ConstructorWithValidParameters) {
    EXPECT_TRUE(calculator->IsOK());
    EXPECT_EQ(calculator->getGasMixture(), gas_mixture_type::CO2);
    EXPECT_DOUBLE_EQ(calculator->getGasPressure(), 250.0);
    EXPECT_DOUBLE_EQ(calculator->getGasTemperature(), 293.15);
}

TEST(IonRangeCalculatorDeathTest, ConstructorWithInvalidGas) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type(99), 250.0, 293.15);
    }, "ERROR: Wrong gas mixture index");
}

// Gas Conditions Tests
TEST_F(IonRangeCalculatorTest, SetGasConditions) {
    calculator->setGasConditions(gas_mixture_type::CO2, 200.0, 283.15);
    
    EXPECT_EQ(calculator->getGasMixture(), gas_mixture_type::CO2);
    EXPECT_DOUBLE_EQ(calculator->getGasPressure(), 200.0);
    EXPECT_DOUBLE_EQ(calculator->getGasTemperature(), 283.15);
    
    std::tuple<gas_mixture_type, double, double> conditions = calculator->getGasConditions();
    EXPECT_EQ(std::get<0>(conditions), gas_mixture_type::CO2);
    EXPECT_DOUBLE_EQ(std::get<1>(conditions), 200.0);
    EXPECT_DOUBLE_EQ(std::get<2>(conditions), 283.15);
}


TEST(IonRangeCalculatorDeathTest, SetInvalidPressure) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.setGasPressure(-10.0);
    }, "ERROR: Wrong gas pressure");
}

TEST(IonRangeCalculatorDeathTest, SetInvalidTemperature) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.setGasTemperature(-10.0);
    }, "ERROR: Wrong gas temperature");
}

// Range Calculation Tests
TEST_F(IonRangeCalculatorTest, GetIonRangeForValidEnergy) {
    double range_proton = calculator->getIonRangeMM(pid_type::PROTON, 1.0);
    EXPECT_THAT(range_proton, Gt(0.0));
    
    double range_alpha = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    EXPECT_THAT(range_alpha, Gt(0.0));
    
    double range_carbon = calculator->getIonRangeMM(pid_type::CARBON_12, 2.5);
    EXPECT_THAT(range_carbon, Gt(0.0));
}

TEST_F(IonRangeCalculatorTest, RangeIncreasesWithEnergy) {
    double range1 = calculator->getIonRangeMM(pid_type::ALPHA, 1.0);
    double range2 = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    
    EXPECT_THAT(range2, Gt(range1));
}

TEST(IonRangeCalculatorDeathTest, GetRangeForNegativeEnergy) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.getIonRangeMM(pid_type::PROTON, -1.0);
    }, "ERROR: Wrong energy");
}

// Energy Calculation Tests
TEST_F(IonRangeCalculatorTest, GetIonEnergyForValidRange) {
    double energy_proton = calculator->getIonEnergyMeV(pid_type::PROTON, 5.0);
    EXPECT_THAT(energy_proton, Gt(0.0));
    
    double energy_alpha = calculator->getIonEnergyMeV(pid_type::ALPHA, 3.5);
    EXPECT_THAT(energy_alpha, Gt(0.0));
}

TEST_F(IonRangeCalculatorTest, RangeEnergyConsistency) {
    double energy = 2.0; // MeV
    double range = calculator->getIonRangeMM(pid_type::PROTON, energy);
    double recovered_energy = calculator->getIonEnergyMeV(pid_type::PROTON, range);
    
    EXPECT_THAT(recovered_energy, DoubleNear(energy, 0.1));
}

TEST(IonRangeCalculatorDeathTest, GetEnergyForNegativeRange) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.getIonEnergyMeV(pid_type::PROTON, -1.0);
    }, "ERROR: Wrong range");
}

// Pressure and Temperature Scaling Tests
TEST_F(IonRangeCalculatorTest, PressureScaling) {
    double range_250mbar = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    
    calculator->setGasPressure(500.0); // Double pressure
    double range_500mbar = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    
    // Range should be approximately halved at double pressure
    EXPECT_THAT(range_500mbar, DoubleNear(range_250mbar * 0.5, range_250mbar * 0.1));
}

TEST_F(IonRangeCalculatorTest, TemperatureScaling) {
    double range_293K = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    
    calculator->setGasTemperature(586.3); // Double temperature
    double range_586K = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    
    // Range should be approximately doubled at double temperature
    EXPECT_THAT(range_586K, DoubleNear(range_293K * 2.0, range_293K * 0.1));
}

// Bragg Curve Tests
TEST_F(IonRangeCalculatorTest, GetBraggCurve) {
    TGraph bragg_curve = calculator->getIonBraggCurveMeVPerMM(pid_type::ALPHA, 5.0, 10);
    
    EXPECT_EQ(bragg_curve.GetN(), 10);
    
    // Check that dE/dx values are positive
    for (int i = 0; i < bragg_curve.GetN(); ++i) {
        double x, y;
        bragg_curve.GetPoint(i, x, y);
        EXPECT_THAT(y, Gt(0.0));
        EXPECT_THAT(x, Gt(-0.001)); // Allow small numerical errors
    }
}

TEST_F(IonRangeCalculatorTest, BraggCurveIntegral) {
    double integral = calculator->getIonBraggCurveIntegralMeV(pid_type::ALPHA, 5.0, 100);
    
    EXPECT_THAT(integral, Gt(0.0));
    // The integral should be close to the initial energy (conservation of energy)
    EXPECT_THAT(integral, DoubleNear(5.0, 1.0));
}

TEST(IonRangeCalculatorDeathTest, BraggCurveInsufficientPoints) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.getIonBraggCurveMeVPerMM(pid_type::ALPHA, 5.0, 1);
    }, "ERROR: Requested Bragg curve with insufficient number of points");
}

// Effective Length Correction Tests
TEST_F(IonRangeCalculatorTest, EffectiveLengthCorrection) {
    double original_range = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    
    // Set length correction: scale = 1.2, offset = 0.5 mm
    calculator->setEffectiveLengthCorrection(pid_type::ALPHA, 1.2, 0.5);
    
    double corrected_range = calculator->getIonRangeMM(pid_type::ALPHA, 5.0);
    double expected_corrected = 1.2 * original_range + 0.5;
    
    EXPECT_THAT(corrected_range, DoubleNear(expected_corrected, 0.01));
    
    EXPECT_THAT(calculator->getEffectiveLengthCorrectionScale(pid_type::ALPHA), DoubleNear(1.2, 1e-6));
    EXPECT_THAT(calculator->getEffectiveLengthCorrectionOffsetMM(pid_type::ALPHA), DoubleNear(0.5, 1e-6));
}

TEST_F(IonRangeCalculatorTest, ResetEffectiveLengthCorrection) {
    calculator->setEffectiveLengthCorrection(pid_type::ALPHA, 1.5, 1.0);
    calculator->resetEffectiveLengthCorrection(pid_type::ALPHA);
    
    EXPECT_THAT(calculator->getEffectiveLengthCorrectionScale(pid_type::ALPHA), DoubleNear(1.0, 1e-6));
    EXPECT_THAT(calculator->getEffectiveLengthCorrectionOffsetMM(pid_type::ALPHA), DoubleNear(0.0, 1e-6));
}

TEST(IonRangeCalculatorDeathTest, ZeroLengthScale) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.setEffectiveLengthCorrection(pid_type::ALPHA, 0.0, 0.5);
    }, "ERROR: Wrong length correction scale");
}

// Reference Conditions Tests
TEST_F(IonRangeCalculatorTest, GetReferenceConditions) {
    double ref_pressure = calculator->getGasRangeReferencePressure(pid_type::ALPHA);
    double ref_temperature = calculator->getGasRangeReferenceTemperature(pid_type::ALPHA);
    
    EXPECT_THAT(ref_pressure, Gt(0.0));
    EXPECT_THAT(ref_temperature, Gt(0.0));
}

// Ion Mass Tests
TEST_F(IonRangeCalculatorTest, GetIonMass) {
    double proton_mass = calculator->getIonMassMeV(pid_type::PROTON);
    double alpha_mass = calculator->getIonMassMeV(pid_type::ALPHA);
    double carbon_mass = calculator->getIonMassMeV(pid_type::CARBON_12);
    
    // Check that masses are reasonable (approximate values in MeV/c²)
    EXPECT_THAT(proton_mass, DoubleNear(938.3, 10.0));   // ~938 MeV/c²
    EXPECT_THAT(alpha_mass, DoubleNear(3727.4, 50.0));   // ~3727 MeV/c²
    EXPECT_THAT(carbon_mass, Gt(10000.0));               // >10 GeV/c²
}

// Edge Cases and Error Conditions
TEST(IonRangeCalculatorDeathTest, UnsupportedIonType) {
    EXPECT_DEATH({
        IonRangeCalculator calc(gas_mixture_type::CO2, 250.0, 293.15);
        calc.getIonRangeMM(static_cast<pid_type>(999), 1.0);
    }, "ERROR: Reference range/energy curve is missing");
}

TEST_F(IonRangeCalculatorTest, IsOKMethod) {
    EXPECT_TRUE(calculator->IsOK());
    
    // Create calculator with invalid conditions
    IonRangeCalculator invalid_calc(gas_mixture_type::CO2, -1.0, -1.0, false);
    EXPECT_FALSE(invalid_calc.IsOK());
}

// Integration Test
TEST_F(IonRangeCalculatorTest, FullWorkflow) {
    // Set specific conditions
    calculator->setGasConditions(gas_mixture_type::CO2, 300.0, 283.15);
    
    // Calculate range for alpha particle
    double energy = 6.0; // MeV
    double range = calculator->getIonRangeMM(pid_type::ALPHA, energy);
    
    // Recover energy from range
    double recovered_energy = calculator->getIonEnergyMeV(pid_type::ALPHA, range);
    
    // Get Bragg curve
    TGraph bragg = calculator->getIonBraggCurveMeVPerMM(pid_type::ALPHA, energy, 50);
    
    // Calculate integral
    double integral = calculator->getIonBraggCurveIntegralMeV(pid_type::ALPHA, energy, 50);
    
    // Verify consistency
    EXPECT_THAT(recovered_energy, DoubleNear(energy, 0.2));
    EXPECT_THAT(range, Gt(0.0));
    EXPECT_EQ(bragg.GetN(), 50);
    EXPECT_THAT(integral, Gt(0.0));
}