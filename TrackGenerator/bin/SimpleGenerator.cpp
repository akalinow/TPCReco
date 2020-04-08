#include <iostream>
#include "LineGenerator.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
/*

*/

namespace pt = boost::property_tree;

template <typename T>
T jsonGet(const pt::ptree::value_type& child, std::string str) {
	boost::optional<T> v = child.second.get_optional<T>(str);
	return v.is_initialized() ? v.get() : static_cast<T>(0);
}

int main(int argc, char* argv[]) {
	pt::ptree root;


	if (argc == 2) {
		//l.loadGeometry(argv[1]);
		pt::read_json(argv[1], root);
	}
	else {
		std::cout << "Usage:\n\tEventGenerator <config.json>" << std::endl;
		return 1;
	}

	LineGenerator l;
	l.loadGeometry(root.get<std::string>("geometryFile"));
	BOOST_FOREACH(const pt::ptree::value_type & child, root.get_child("lines")) {
		auto x0 = jsonGet<double>(child, "x0");
		auto y0 = jsonGet<double>(child, "y0");
		auto z0 = jsonGet<double>(child, "z0");
		auto length = jsonGet<double>(child, "length");
		auto theta = jsonGet<double>(child, "theta");
		auto phi = jsonGet<double>(child, "phi");
		auto randomLength = jsonGet<bool>(child, "lengthRandom");
		auto randomTheta = jsonGet<bool>(child, "thetaRandom");
		auto randomPhi = jsonGet<bool>(child, "phiRandom");
		std::cout << randomTheta << std::endl;
		l.addLine(length, theta, phi, TVector3(x0, y0, z0), randomTheta, randomPhi, randomLength);
	}
	l.setTrackCounts(root.get<double>("MCcounts"));
	l.setTrackSigma(root.get<double>("sigma"));
	l.setTrackSpace(root.get<int>("NbinsX"), root.get<double>("xmin"), root.get<double>("xmax"),
		root.get<int>("NbinsY"), root.get<double>("ymin"), root.get<double>("ymax"),
		root.get<int>("NbinsZ"), root.get<double>("zmin"), root.get<double>("zmax"));
	l.setOutput(root.get<std::string>("outputFile"));
	l.generateEvents(root.get<int>("events"));
	l.writeOutput();
	l.closeOutput();
	return 0;
}