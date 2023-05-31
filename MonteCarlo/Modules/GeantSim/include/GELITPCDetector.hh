/**
 * @file GELITPCDetector.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELITPCDetector class
 */

#ifndef GELITPCDETECTOR_H
#define GELITPCDETECTOR_H

class G4LogicalVolume;

/**
 * @brief      Class represents TPC detector
 */
class GELITPCDetector {
public:

    /**
     * @brief      Builds TPC detector
     * @details    mother_log is used as a mother volume for the detector
     */
    static void BuildTPCDetector(G4LogicalVolume *mother_log);

};

#endif