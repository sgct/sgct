/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SHADER_DATA__H__
#define __SGCT__SHADER_DATA__H__

#include <sgct/Shader.h>

namespace sgct_core {

struct ShaderData {
    Shader mShader;
    std::string mShaderSrc;
    bool mIsSrcFile;
};

} // namespace sgct_core

#endif // __SGCT__SHADER_DATA__H__
