/* ReadConfig.h

© 2012 Miroslav Andel

*/

#ifndef _READ_CONFIG
#define _READ_CONFIG

#include <string>
#include <vector>

#include "Point3.h"


struct NodeConfig
{
	bool master;
	bool useSwapGroups;
	std::string ip;
	bool fullscreen;
	int numberOfSamples;
	int stereo;
	int windowData[4]; //offset x y and size x y
	Point3f viewPlaneCoords[3];
};

class ReadConfig
{
public:
	ReadConfig( const std::string filename );

	enum StereoMode { None = 0, Active, PassiveVertical, PassiveHorizontal, Checkerboard };
	enum corners { LowerLeft = 0, UpperLeft, UpperRight };

	std::string * getMasterIP() { return &masterIP; }
	std::string * getMasterPort() { return &masterPort; }
	NodeConfig * getNodePtr(unsigned int index) { return &nodes[index]; }
	unsigned int getNumberOfNodes() { return nodes.size(); }
	bool isValid() { return valid; }
	Point3f * getUserPos() { return &userPos; }
	float getEyeSeparation() { return eyeSeparation; }

private:
	void readAndParseXML();
	int getStereoType( const std::string type );

	bool valid;
	std::string xmlFileName;
	std::vector<NodeConfig> nodes;
	Point3f userPos;
	float eyeSeparation;

	std::string masterIP;
	std::string masterPort;
};

#endif