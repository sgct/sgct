/* ReadConfig.h

© 2012 Miroslav Andel

*/

#ifndef _SGCT_READ_CONFIG
#define _SGCT_READ_CONFIG

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace core_sgct //simple graphics cluster toolkit
{

class ReadConfig
{
public:
	ReadConfig( const std::string filename );

	enum StereoMode { None = 0, Active, Checkerboard };

	bool isValid() { return valid; }
	bool isExternalControlPortSet() { return useExternalControlPort; }
	bool isMasterSyncLocked() { return useMasterSyncLock; }
	glm::vec3 * getSceneOffset() { return &sceneOffset; }

private:
	void readAndParseXML();
	int getStereoType( const std::string type );

	bool valid;
	bool useMasterSyncLock;
	bool useExternalControlPort;
	std::string xmlFileName;
	glm::vec3 sceneOffset;
};

}

#endif
