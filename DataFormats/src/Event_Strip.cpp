#include "Event_Strip.h"

double& Event_Strip::operator[](int index) {
	return chargeByTimeCell[index];
}