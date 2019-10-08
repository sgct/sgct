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
    /// Enum for deciding shader type
    using ShaderType = GLenum;

    Shader() = default;

    /**
     * The constructor sets shader type.
     *
     * \param shaderType The shader type: GL_COMPUTE_SHADER, GL_VERTEX_SHADER,
     *        GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or 
     *        GL_FRAGMENT_SHADER
     */

    Shader(ShaderType shaderType);

    /**
     * Set the shader type.
     *
     * \param shaderType The shader type: GL_COMPUTE_SHADER, GL_VERTEX_SHADER,
     *        GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or
     *        GL_FRAGMENT_SHADER
     */
    void setShaderType(ShaderType shaderType);

    /**
     * Set the shader source code from a file, will create and compile the shader if it is
     * not already done. At this point a compiled shader can't have its source reset.
     * Recompilation of shaders is not supported.
     *
     * \param file Path to shader file
     *
     * \return If setting source and compilation went ok.
     */
    bool setSourceFromFile(const std::string& file);
    
    /**
     * Set the shader source code from a file, will create and compile the shader if it is
     * not already done. At this point a compiled shader can't have its source reset.
     * Recompilation of shaders is not supported.
     *
     * \param sourceString String with shader source code
     *
     * \return If setting the source and compilation went ok.
     */
    bool setSourceFromString(const std::string& srcString);

    /**
     * Will return the name of the shader type.
     *
     * \param shaderType The shader type
     *
     * \return Shader type name
     */
    std::string getShaderTypeName(ShaderType shaderType) const;

    /**
     * Get the id for this shader used for linking against programs. The shader source
     * must be set before the id can be used. The shader won't be created until it has a
     * source.
     *
     * \return Shader id that can be used for program linking
     */
    int getId() const;

    /// Delete the shader
    void deleteShader();

private:
    /**
     * Will check the compilation status of the shader and output any errors from the
     * shader log.
     *
     * \return Status of the compilation
     */
    bool checkCompilationStatus() const;

    ShaderType _shaderType; // The shader type
    int _shaderId = 0;      // The shader _id used for reference
};

} // sgct_core

#endif // __SGCT__SHADER__H__
