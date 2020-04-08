#ifndef _LineGenerator_H_
#define _LineGenerator_H_
#include "AbstractGenerator.h"
#include "Line.h"
#include <TRandom3.h>
#include <TVector3.h>
#include <memory>
#include <vector>
/*Builder of EventsTPC from lines
* LineGenerator g;
* g.loadGeometry(path/to/geometry);
* g.setTrackOrigin/Length/...(arguments);
* g.setOutput(path/to/output);
* g.addLine(length,theta,phi);
* g.generateEvents();
* g.writeOutput();
* g.closeOutput();
*/
class LineGenerator : public AbstractGenerator {
public:
	//Default constructor (not RAII)
	LineGenerator() = default;

	//Generates track 3d histogram for line contructed with setters
	void generateTrack();

	//Generates events in loop
	void generateEvents(int count = 0) final;
	void addLine(TVector3 stop, TVector3 start = TVector3(0, 0, 0), bool randomTheta = false, bool randomPhi = false, bool randomLength = false)
	{
		Line line(&rng, stop, start); line.setRandom(randomTheta, randomPhi, randomLength); lines.push_back(line);
	}
	void addLine(double length, double theta, double phi, TVector3 start = TVector3(0, 0, 0), bool randomTheta = false, bool randomPhi = false, bool randomLength = false)
	{
		Line line(&rng, length, theta, phi, start); line.setRandom(randomTheta, randomPhi, randomLength); lines.push_back(line);
	}
	void addLine(Line& line) { lines.push_back(line); }
	//Set-up
/*
	// Sets origin point of track in mm
	inline void setTrackOrigin(double x=0,double y=0,double z=0){trackOrigin=TVector3(x,y,z);}

	// Sets length of track in mm
	inline void setTrackLength(double length=1){trackVector.SetMag(length);}

	// Sets track's phi (angle in XY/readout plane) in rad.
	inline void setTrackPhi(double phi=0){trackVector.SetPhi(phi);}

	// Sets track's theta (counting from Z) in rad.
	inline void setTrackTheta(double theta=0){trackVector.SetTheta(theta);}
*/
// Sets gaussian spread of track in mm. The spread is same in all directions.
	inline void setTrackSigma(double sigma) { trackSigma = sigma; }

	// Sets number of entries for generating track.
	inline void setTrackCounts(int counts) { trackCounts = counts; }

private:
	TRandom3 rng;
	//    double trackLength{100}; //mm
	//    double trackTheta{0.6}; //deg from z/t axis
	//    double trackPhi{1.1};  //deg in UWV plane
	//    TVector3 trackOrigin{0,0,0}; // mm/mm/mm
	//    TVector3 trackVector{1,0,0};
	double trackSigma{ 3 }; //mm
	double trackCounts{ 10000 }; //mc counts
	std::vector<Line> lines;
};
#endif // _LineGenerator_H_