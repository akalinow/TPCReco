/**
 * @file GELITabulatedField3D.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELITabulatedField3D class
 */

#include "globals.hh"
#include "G4MagneticField.hh"
#include "G4ios.hh"

/// \cond
#include <fstream>
#include <vector>
#include <cmath>
/// \endcond

using namespace std;

/**
 * @brief      Class handles interface for magnetic field map
 * @details    It reads magnetic field map from the file, stores it in memory
 *             and handles interpolation for tracking purposes
 */
class GELITabulatedField3D : public G4MagneticField
{
  vector< vector< vector< G4double > > > xField; ///< Storage for x coordinate od magnetic field
  vector< vector< vector< G4double > > > yField; ///< Storage for y coordinate od magnetic field
  vector< vector< vector< G4double > > > zField; ///< Storage for z coordinate od magnetic field
  // The dimensions of the table
  int nx; ///<Number of bins in x
  int ny; ///<Number of bins in y
  int nz; ///<Number of bins in z
  // The physical limits of the defined region
  double minx; ///< Low limit for x coordinate
  double maxx; ///< Up limit for x coordinate
  double miny; ///< Low limit for y coordinate
  double maxy; ///< Up limit for y coordinate
  double minz; ///< Low limit for z coordinate
  double maxz; ///< Up limit for z coordinate
  // The physical extent of the defined region
  double dx; ///< Width of the bin in x coordinate
  double dy; ///< Width of the bin in y coordinate
  double dz; ///< Width of the bin in z coordinate
  double fXoffset; ///< Offset of the magnetic field in x coordinate
  bool invertX; ///< Flag to tell if x coordinate should be inverted
  bool invertY; ///< Flag to tell if y coordinate should be inverted
  bool invertZ; ///< Flag to tell if z coordinate should be inverted

public:

  /**
   * @brief      Constructor
   */
  GELITabulatedField3D(const char* filename, G4double xOffset );
  
  /**
   * @brief      Access to value of the magnetic field used for tracking
   */
  void  GetFieldValue( const  double Point[4],
		       double *Bfield          ) const;
};

