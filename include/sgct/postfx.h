/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__POST_FX__H__
#define __SGCT__POST_FX__H__

#include <sgct/shaderprogram.h>
#include <glm/glm.hpp>

namespace sgct {

/**
 * Class that holds a post effect pass
 */
class PostFX {
public:
    /// \returns true if shader and output/target texture created successfully
    bool init(std::string name, const std::string& vertShaderSrc,
        const std::string& fragShaderSrc,
        ShaderProgram::ShaderSourceType srcType = ShaderProgram::ShaderSourceType::File);
    void destroy();

    /// Render this pass
    void render();
    void setUpdateUniformsFunction(void(*fnPtr)());
    void setInputTexture(unsigned int inputTex);
    void setOutputTexture(unsigned int outputTex);
    
    /// \returns the output texture
    unsigned int getOutputTexture() const;
    
    /// \returns the input texture
    unsigned int getInputTexture() const;
    
    /// \returns the shader pointer
    ShaderProgram& getShaderProgram();

    /// \returns the shader pointer
    const ShaderProgram& getShaderProgram() const;
    
    /// \returns name of this post effect pass
    const std::string& getName() const;

private:
    void internalRender();
    void internalRenderFixedPipeline();

    void (*mUpdateFn)() = nullptr;
    void (PostFX::*mRenderFn)(void) = nullptr;

    ShaderProgram mShaderProgram;
    unsigned int mInputTexture = 0;
    unsigned int mOutputTexture = 0;
    
    glm::ivec2 mSize = glm::ivec2(1, 1);
    std::string mName;
    inline static bool mDeleted = false;
};

} // namespace sgct

#endif // __SGCT__POST_FX__H__
