/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__POST_FX__H__
#define __SGCT__POST_FX__H__

#include <sgct/shaderprogram.h>
#include <glm/glm.hpp>
#include <functional>

namespace sgct {

/**
 * Class that holds a post effect pass
 */
class PostFX {
public:
    /// \return true if shader and output/target texture created successfully
    PostFX(std::string name, const std::string& vertShaderSrc,
        const std::string& fragShaderSrc, std::function<void()>);
    ~PostFX();
    PostFX(PostFX&& rhs) noexcept;

    PostFX& operator=(PostFX&& rhs) noexcept = default;

    /// Render this pass
    void render();
    void setInputTexture(unsigned int inputTex);
    void setOutputTexture(unsigned int outputTex);
    
    /// \return the output texture
    unsigned int getOutputTexture() const;
    
    /// \return the input texture
    unsigned int getInputTexture() const;
    
    /// \return the shader pointer
    const ShaderProgram& getShaderProgram() const;
    
    /// \return name of this post effect pass
    const std::string& getName() const;

private:
    std::function<void()> _updateFunction;

    ShaderProgram _shaderProgram;
    unsigned int _inputTexture = 0;
    unsigned int _outputTexture = 0;

    glm::ivec2 _size = glm::ivec2(1, 1);
    std::string _name;
};

} // namespace sgct

#endif // __SGCT__POST_FX__H__
