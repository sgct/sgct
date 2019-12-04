/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SHADER__H__
#define __SGCT__SHADER__H__

#include <sgct/ogl_headers.h>
#include <string>

namespace sgct::core {

/**
 * Simple helper class for handling shaders. Shader can't be used directly, they must be
 * linked to a program. Current implementation only supports vertex and fragment shader.
 */
class Shader {
public:
    /**
     * Set the shader type and the shader source code.
     *
     * \param shaderType The shader type: GL_COMPUTE_SHADER, GL_VERTEX_SHADER,
     *        GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or
     *        GL_FRAGMENT_SHADER
     * \param sourceString String with shader source code
     */
    Shader(GLenum shaderType, const std::string& sourceString);

    Shader(const Shader& rhs);
    Shader(Shader&& rhs) noexcept;

    Shader& operator=(const Shader& rhs) noexcept;
    Shader& operator=(Shader&& rhs) noexcept;

    /// Deletes the shader
    ~Shader();

    /**
     * Gets the id for this shader. The shader source must be set before the id can be
     * used. The shader won't be created until it has a source.
     *
     * \return Shader id that can be used for program linking
     */
    int id() const;

private:
    GLenum _shaderType; // The shader type
    int _shaderId = 0;  // The shader _id used for reference
};

} // namespace sgct::core

#endif // __SGCT__SHADER__H__
