/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_READ_CONFIG
#define _SGCT_READ_CONFIG

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "SGCTWindow.h"

namespace sgct_core //simple graphics cluster toolkit
{

class ReadConfig
{
public:
	ReadConfig( const std::string filename );

	bool isValid() { return valid; }
	static glm::quat parseOrientationNode(tinyxml2::XMLElement* element);

private:
    bool replaceEnvVars( const std::string &filename );
	bool readAndParseXML();
	sgct::SGCTWindow::StereoMode getStereoType( std::string type );
	sgct::SGCTWindow::ColorBitDepth getBufferColorBitDepth(std::string type);

	bool valid;
	std::string xmlFileName;
	std::string mErrorMsg;
};

}

#endif
