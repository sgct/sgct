/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__POSTFX__H__
#define __SGCT__POSTFX__H__

#include <sgct/shaderprogram.h>
#include <functional>

namespace sgct {

class Window;

/// Class that holds a post effect pass
class PostFX {
public:
    /// \return true if shader and output/target texture created successfully
    PostFX(std::string name, const std::string& vertShaderSrc,
        const std::string& fragShaderSrc, std::function<void()>);
    ~PostFX();
    PostFX(PostFX&& rhs) noexcept;

    PostFX& operator=(PostFX&& rhs) noexcept = default;

    /// Render this pass
    void render(Window& window);
    void setInputTexture(unsigned int inputTex);
    void setOutputTexture(unsigned int outputTex);
    
    /// \return the output texture
    unsigned int getOutputTexture() const;
    
    /// \return the input texture
    unsigned int getInputTexture() const;
    
    /// \return the shader pointer
    const ShaderProgram& getShaderProgram() const;
    
private:
    std::function<void()> _updateFunction;

    ShaderProgram _shaderProgram;
    unsigned int _inputTexture = 0;
    unsigned int _outputTexture = 0;
};

} // namespace sgct

#endif // __SGCT__POSTFX__H__
