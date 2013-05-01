/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_READ_CONFIG
#define _SGCT_READ_CONFIG

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace sgct_core //simple graphics cluster toolkit
{

class ReadConfig
{
public:
	ReadConfig( const std::string filename );

	bool isValid() { return valid; }
	bool isExternalControlPortSet() { return useExternalControlPort; }

	//font stuff
	const int & getFontSize() { return mFontSize; }
	const std::string getFontName() { return mFontName; }
	const std::string getFontPath() { return mFontPath; }

private:
    bool replaceEnvVars( const std::string &filename );
	void readAndParseXML();
	int getStereoType( const std::string type );
	int getFisheyeCubemapRes( const std::string quality );

	bool valid;
	bool useExternalControlPort;
	std::string xmlFileName;
	//fontdata
	std::string mFontName;
	std::string mFontPath;
	int mFontSize;
};

}

#endif
