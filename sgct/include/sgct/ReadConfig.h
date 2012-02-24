/* ReadConfig.h

© 2012 Miroslav Andel

*/

#ifndef _READ_CONFIG
#define _READ_CONFIG

#include <string>
#include <vector>
#include "Point3.h"

namespace core_sgct //simple graphics cluster toolkit
{

class ReadConfig
{
public:
	ReadConfig( const std::string filename );

	enum StereoMode { None = 0, Active, PassiveVertical, PassiveHorizontal, Checkerboard };

	bool isValid() { return valid; }
	bool isExternalControlPortSet() { return useExternalControlPort; }
	Point3f * getUserPos() { return &userPos; }
	float getEyeSeparation() { return eyeSeparation; }

private:
	void readAndParseXML();
	int getStereoType( const std::string type );

	bool valid;
	bool useExternalControlPort;
	std::string xmlFileName;
	Point3f userPos;
	float eyeSeparation;
};

}

#endif