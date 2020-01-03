#ifndef _Hit2D_H_
#define _Hit2D_H_

#include <vector>
#include <memory>

#include "CommonDefinitions.h"

struct i_Hit2D {
	double posWire, posTime;
	double charge;
};

class Hit2D {

public:

	Hit2D(double aPosTime, double aPosWire, double aCharge) : Hit2D_data{ aPosWire, aPosTime, aCharge } { }
	
	auto operator()() const { return Hit2D_data; };
private:

	i_Hit2D Hit2D_data;
};

typedef std::vector<Hit2D> Hit2DCollection;

#endif

