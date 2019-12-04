/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SHADERPROGRAM__H__
#define __SGCT__SHADERPROGRAM__H__

#include <sgct/shader.h>
#include <vector>

namespace sgct {

/**
 * Class for handling compiling, linking and using shader programs. Uniform and attribute
 * handling must be managed explicitly.
*/
class ShaderProgram {
public:
    /**
     * Default only sets the program name. Shaders objects won't be created until any
     * shader source code is set. The program will be created when the createAndLink()
     * function is called. Make sure the shader sources are set before calling it.
     *
     * \param name Name of the shader program. Must be unique
     */
    ShaderProgram() = default;
    ShaderProgram(std::string name);
    ShaderProgram(ShaderProgram&&) noexcept;

    /**
     * The destructor clears the shader data vector but the program can still be used. The
     * program have to be destroyed explicitly by calling deleteProgram. This is so that
     * programs can be copied when storing in containers.
     */
    ~ShaderProgram() = default;

    ShaderProgram& operator=(ShaderProgram&&) noexcept;

    /// Will detach all attached shaders, delete them and then delete the program
    void deleteProgram();

    /**
     * Will create and add a shader to the program.
     *
     * \param src The shader source string
     * \param type Type of shader can be one of the following: GL_COMPUTE_SHADER,
     *             GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER,
     *             GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER
     * \throws std::runtime_error If the adding of the shaders failed
     */
    void addShaderSource(std::string src, GLenum type);

    /**
     * Creates and adds a vertex and fragment shader and adds them to the shader program.
     * 
     * \param vertexSrc Source text for the vertex program
     * \param fragmentSrc Source text for the fragment program
     * \throws std::runtime_error If the adding of the shaders failed
     */
    void addShaderSource(std::string vertexSrc, std::string fragmentSrc);

    /**
     * Will create the program and link the shaders. The shader sources must have been set
     * before the program can be linked. After the program is created and linked no
     * modification to the shader sources can be made.
     *
     * \return Whether the program was created and linked correctly or not
     */
    void createAndLinkProgram();
    
    /// Use the shader program in the current rendering pipeline
    void bind() const;

    /// Unset the shader program in the current rendering pipeline
    static void unbind();

    /// Get the name of the program
    std::string name() const;

    /// Check if the program is linked
    bool isLinked() const;

    /// Get the program ID
    int id() const;

private:
    /// Will create and the program and return whether it was properly created or not
    bool createProgram();
    
    std::string _name = "SGCT_NULL"; /// Name of the program, has to be unique
    bool _isLinked = false;          /// If this program has been linked
    int _programId = 0;              /// Unique program _id

    std::vector<core::Shader> _shaders;
};

} // namespace sgct

#endif // __SGCT__SHADERPROGRAM__H__
