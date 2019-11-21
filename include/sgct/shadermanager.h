/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SHADER_MANAGER__H__
#define __SGCT__SHADER_MANAGER__H__

#include <sgct/shaderprogram.h>
#include <string>
#include <vector>

namespace sgct {

/// For managing shader programs. Implemented as a singleton
class ShaderManager {
public:
    static ShaderManager& instance();
    static void destroy();

    /// Destructor deallocates and deletes all shaders
    ~ShaderManager();

    /**
     * Adds shader programs to the manager. The shaders will be compiled and linked to the
     * program. The name of the shader needs to be unique or it won't be added and an
     * exception is thrown. Both vertex shader and fragment shader source need to be
     * provided, either as a link to a shader source code file or as shader source code.
     *
     * \param name Unique name of the shader
     * \param vertexSrc The vertex shader source code
     * \param fragmentSrc The fragment shader source code
     * \param geometrySrc The geometry shader source code
     * \throws std::runtime_error If there was an error creating the shader program
     */
    void addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc, const std::string& geometrySrc = "");

    /**
     * Removes a shader program from the manager. All resources allocated for the program
     * will be deallocated and removed.
     *
     * \param name Name of the shader program to remove
     * \return true if the shader program was removed correctly
     */
    bool removeShaderProgram(const std::string& name);

    /**
     * Check if a shader program exists in the manager.
     *
     * \param name Name of the shader program
     */
    bool shaderProgramExists(const std::string& name) const;
    
    /**
     * Get the specified shader program from the shader manager.
     *
     * \param name Name of the shader program
     * \return The specified shader program
     * \throws std::runtime_error If the shader program with the \p name was not found
     */
    const ShaderProgram& getShaderProgram(const std::string& name) const;

private:
    static ShaderManager* _instance;
    // Active shaders in the manager
    std::vector<ShaderProgram> _shaderPrograms;
};

} // namespace sgct

#endif // __SGCT__SHADER_MANAGER__H__
