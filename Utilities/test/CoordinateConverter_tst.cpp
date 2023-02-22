#include "TPCReco/CoordinateConverter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <TMath.h>
#include <TVector3.h>
#include <iostream>


class CoorindateConverterBaseTest : public ::testing::Test {
public:
  TVector3 x{1, 0, 0};
  TVector3 y{0, 1, 0};
  TVector3 z{0, 0, 1};
  double precision{1e-15};
};

class CoorindateConverterIdentityTest : public CoorindateConverterBaseTest {
public:
  CoordinateConverter converter{{0,0,0}, {}};
};

TEST_F(CoorindateConverterIdentityTest, DET2BEAM) {
  // X_DET -> X_BEAM
  EXPECT_NEAR(converter.detToBeam(x).X(), x.X(), precision);
  EXPECT_NEAR(converter.detToBeam(x).Y(), x.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(x).Z(), x.Z(), precision);
  // Y_DET -> Y_BEAM
  EXPECT_NEAR(converter.detToBeam(y).X(), y.X(), precision);
  EXPECT_NEAR(converter.detToBeam(y).Y(), y.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(y).Z(), y.Z(), precision);
  // Z_DET -> Z_BEAM
  EXPECT_NEAR(converter.detToBeam(z).X(), z.X(), precision);
  EXPECT_NEAR(converter.detToBeam(z).Y(), z.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(z).Z(), z.Z(), precision);
}

TEST_F(CoorindateConverterIdentityTest, BEAM2DET) {
  // X_BEAM -> X_DET
  EXPECT_NEAR(converter.beamToDet(x).X(), x.X(), precision);
  EXPECT_NEAR(converter.beamToDet(x).Y(), x.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(x).Z(), x.Z(), precision);
  // Y_BEAM -> Y_DET
  EXPECT_NEAR(converter.beamToDet(y).X(), y.X(), precision);
  EXPECT_NEAR(converter.beamToDet(y).Y(), y.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(y).Z(), y.Z(), precision);
  // Z_BEAM -> Z_DET
  EXPECT_NEAR(converter.beamToDet(z).X(), z.X(), precision);
  EXPECT_NEAR(converter.beamToDet(z).Y(), z.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(z).Z(), z.Z(), precision);
}


class CoorindateConverterTest : public CoorindateConverterBaseTest {
public:
  CoordinateConverter converter{{-M_PI / 2.0, M_PI / 2.0, 0.0}, {}};
};

TEST_F(CoorindateConverterTest, DET2BEAMx) {
  // X_DET -> -Z_BEAM
  EXPECT_NEAR(converter.detToBeam(x).X(), -z.X(), precision);
  EXPECT_NEAR(converter.detToBeam(x).Y(), -z.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(x).Z(), -z.Z(), precision);
}

TEST_F(CoorindateConverterTest, DET2BEAMy) {
  // Y_DET ->  X_BEAM
  EXPECT_NEAR(converter.detToBeam(y).X(), x.X(), precision);
  EXPECT_NEAR(converter.detToBeam(y).Y(), x.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(y).Z(), x.Z(), precision);
}

TEST_F(CoorindateConverterTest, DET2BEAMz) {
  // Z_DET -> -Y_BEAM
  EXPECT_NEAR(converter.detToBeam(z).X(), -y.X(), precision);
  EXPECT_NEAR(converter.detToBeam(z).Y(), -y.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(z).Z(), -y.Z(), precision);
}

TEST_F(CoorindateConverterTest, BEAM2DETx) {
  // X_BEAM ->  Y_DET
  EXPECT_NEAR(converter.beamToDet(x).X(), y.X(), precision);
  EXPECT_NEAR(converter.beamToDet(x).Y(), y.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(x).Z(), y.Z(), precision);
}

TEST_F(CoorindateConverterTest, BEAM2DETy) {
  // Y_BEAM -> -Z_DET
  EXPECT_NEAR(converter.beamToDet(y).X(), -z.X(), precision);
  EXPECT_NEAR(converter.beamToDet(y).Y(), -z.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(y).Z(), -z.Z(), precision);
}

TEST_F(CoorindateConverterTest, BEAM2DETz) {
  // Z_BEAM -> -X_DET
  EXPECT_NEAR(converter.beamToDet(z).X(), -x.X(), precision);
  EXPECT_NEAR(converter.beamToDet(z).Y(), -x.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(z).Z(), -x.Z(), precision);
}


class CoorindateConverterCorrectionTest : public CoorindateConverterBaseTest {
public:
  CoordinateConverter converter{{}, {-M_PI / 2.0, M_PI / 2.0, 0.0}};
};

TEST_F(CoorindateConverterCorrectionTest, DET2BEAMx) {
  // X_DET -> -Z_BEAM
  EXPECT_NEAR(converter.detToBeam(x).X(), -z.X(), precision);
  EXPECT_NEAR(converter.detToBeam(x).Y(), -z.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(x).Z(), -z.Z(), precision);
}

TEST_F(CoorindateConverterCorrectionTest, DET2BEAMy) {
  // Y_DET ->  X_BEAM
  EXPECT_NEAR(converter.detToBeam(y).X(), x.X(), precision);
  EXPECT_NEAR(converter.detToBeam(y).Y(), x.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(y).Z(), x.Z(), precision);
}

TEST_F(CoorindateConverterCorrectionTest, DET2BEAMz) {
  // Z_DET -> -Y_BEAM
  EXPECT_NEAR(converter.detToBeam(z).X(), -y.X(), precision);
  EXPECT_NEAR(converter.detToBeam(z).Y(), -y.Y(), precision);
  EXPECT_NEAR(converter.detToBeam(z).Z(), -y.Z(), precision);
}

TEST_F(CoorindateConverterCorrectionTest, BEAM2DETx) {
  // X_BEAM ->  Y_DET
  EXPECT_NEAR(converter.beamToDet(x).X(), y.X(), precision);
  EXPECT_NEAR(converter.beamToDet(x).Y(), y.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(x).Z(), y.Z(), precision);
}

TEST_F(CoorindateConverterCorrectionTest, BEAM2DETy) {
  // Y_BEAM -> -Z_DET
  EXPECT_NEAR(converter.beamToDet(y).X(), -z.X(), precision);
  EXPECT_NEAR(converter.beamToDet(y).Y(), -z.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(y).Z(), -z.Z(), precision);
}

TEST_F(CoorindateConverterCorrectionTest, BEAM2DETz) {
  // Z_BEAM -> -X_DET
  EXPECT_NEAR(converter.beamToDet(z).X(), -x.X(), precision);
  EXPECT_NEAR(converter.beamToDet(z).Y(), -x.Y(), precision);
  EXPECT_NEAR(converter.beamToDet(z).Z(), -x.Z(), precision);
}

TEST(ConverterTest, cout){
    std::cout<<CoordinateConverter{{-M_PI / 2.0, M_PI / 2.0, 0.0}, {}};
}