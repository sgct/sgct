/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SHADER_DATA_H_
#define _SHADER_DATA_H_

#include "Shader.h"
#include "ShaderProgram.h"

namespace sgct_core
{

class ShaderData
{
public:
	Shader mShader;
	std::string mShaderSrc;
	bool mIsSrcFile;
};

}

#endif
