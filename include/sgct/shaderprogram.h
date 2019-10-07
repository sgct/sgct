/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SHADER_PROGRAM__H__
#define __SGCT__SHADER_PROGRAM__H__

#include <sgct/shader.h>
#include <sgct/shaderdata.h>
#include <vector>

namespace sgct {

/**
 * Helper class for handling compiling, linking and using shader programs. Current
 * implementation only supports vertex and fragment shader. Uniform and attribute handling
 * must be managed explicitly but it is possible to poll the Shader program for uniform
 * and attribute locations.
*/
class ShaderProgram {
public:
    /// If shader source should be loaded from file or read as is
    enum class ShaderSourceType { File, String };

    /**
     * Default only sets the program name.Shaders objects won't be created until any
     * shader source code is set. The program will be created when the createAndLink()
     * function is called. Make sure the shader sources are set before calling it.
     *
     * \param name Name of the shader program. Must be unique
     */
    ShaderProgram() = default;
    ShaderProgram(std::string name);

    /**
     * The destructor clears the shader data vector but the program can still be used. The
     * program have to be destroyed explicitly by calling deleteProgram. This is so that
     * programs can be copied when storing in containers.
     */
    ~ShaderProgram() = default;

    /// Will detach all attached shaders, delete them and then delete the program
    void deleteProgram();

    /// \param name Name of the shader program
    void setName(std::string name);

    /**
     * Will add a shader to the program.
     *
     * \param src Where the source is found, can be either a file path or shader source
     *            string
     * \param type Type of shader can be one of the following: GL_COMPUTE_SHADER,
     *             GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER,
     *             GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER
     * \param sSrcType What type of source code should be read, file or string
     *
     * \return Whether the source code was set correctly or not
     */
    bool addShaderSrc(std::string src, core::Shader::ShaderType type,
        ShaderSourceType sSrcType = ShaderSourceType::File);

    /**
     * Will create the program and link the shaders. The shader sources must have been set
     * before the program can be linked. After the program is created and linked no
     * modification to the shader sources can be made.
     *
     * \return Whether the program was created and linked correctly or not
     */
    bool createAndLinkProgram();
    
    /**
     * Reloads a shader by deleting, recompiling and re-linking.
     *
     * \return Whether the program was created and linked correctly or not
     */
    bool reload();

    /// Use the shader program in the current rendering pipeline
    bool bind() const;

    /// Unset the shader program in the current rendering pipeline
    static void unbind();

    /**
     * Get the location of the attribute, no explicit error checks are performed. Users
     * are responsible of checking the return value of the attribute location.
     *
     * \param name Name of the attribute
     *
     * \return Uniform location within the program, -1 if not an active attribute
     */
    int getAttribLocation(const std::string& name) const;

    /**
     * Get the location of the attribute, no explicit error checks are performed. Users
     * are responsible of checking the return value of the attribute location.
     *
     * \param name Name of the uniform
     *
     * \return Uniform location within the program, -1 if not an active uniform
     */
    int getUniformLocation(const std::string& name) const;

    /**
     * Wrapper for glBindFragDataLocation
     * Bind a user-defined varying out variable to a fragment shader color number
     *
     * \param colorNumber The color number to bind the user-defined varying variable to
     * \param name The name of the user-defined varying variable whose binding to modify
     */

    void bindFragDataLocation(unsigned int colorNumber, const std::string& name) const;

    /// Get the name of the program
    std::string getName() const;

    /// Check if the program is linked
    bool isLinked() const;

    /// Get the program ID
    int getId() const;

private:
    /**
     * Will create the program.
     *
     * \return Whether the program was properly created or not
     */
    bool createProgram();
    
    /**
     * Will check the link status of the program and output any errors from the program
     * log.
     *
     * \return Status of the compilation
     */
    bool checkLinkStatus() const;

    std::string _name = "SGCT_NULL"; /// Name of the program, has to be unique
    bool _isLinked = false;          /// If this program has been linked
    int _programId = 0;              /// Unique program _id

    std::vector<core::ShaderData> _shaders;
};

} // namespace sgct

#endif // __SGCT__SHADER_PROGRAM__H__
