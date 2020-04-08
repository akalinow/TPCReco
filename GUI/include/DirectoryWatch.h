#ifndef DirectoryWatch_H
#define DirectoryWatch_H

#include "TQObject.h"
#include "RQ_OBJECT.h"

class DirectoryWatch : public TQObject {

	RQ_OBJECT("DirectoryWatch")

public:

	DirectoryWatch() = default;
	virtual ~DirectoryWatch() = default;
	void watch(const std::string& dirName);

};

#endif
