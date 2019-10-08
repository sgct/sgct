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

/**
 * For managing shader programs. Implemented as a singleton
 * The current implementation of shader programs only support vertex and fragment shaders.
 */
class ShaderManager {
public:
    /**
     * A shader program that never will be initialized. Will be returned for not found
     * programs and can be used as comparison for NULL values
     */
    ShaderProgram NullShader = ShaderProgram("SGCT_NULL");

    /// Destructor deallocates and deletes all shaders
    ~ShaderManager();

    /**
     * Add a empty shader program to the manager. This function is used when creating
     * advanced shader programs. Compilation is not automatic in this case.
     *
     * \param name Unique name of the shader
     * \param shaderProgram Reference to ShaderProgram
     *
     * \return true if shader program was added to the shader manager
     */
    bool addShaderProgram(const std::string& name, ShaderProgram& shaderProgram);

    /**
     * Add a shader program to the manager. The shaders will be compiled and linked to the
     * program. The name of the shader needs to be unique or it won't be added. Both
     * vertex shader and fragment shader source need to be provided, either as a link to a
     * shader source code file or as shader source code.
     *
     * \param name Unique name of the shader
     * \param vertexSrc The vertex shader source code, can be a file path or source code
     * \param fragmentSrc The fragment shader source code, can be a file path or source
     *                    code
     * \param sSrcTyp Shader source code type, if it is a link to a file or source code
     *
     * \return Whether the shader was created, linked and added to the manager correctly
     *         or not.
     */
    bool addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::ShaderSourceType::File);

    /**
     * Add a shader program to the manager. The shaders will be compiled and linked to the
     * program. The name of the shader needs to be unique or it won't be added. Both
     * vertex shader and fragment shader source need to be provided, either as a link to a
     * shader source code file or as shader source code.
     *
     * \param shaderProgram Reference to ShaderProgram
     * \param name Unique name of the shader
     * \param vertexSrc The vertex shader source code, can be a file path or source code
     * \param fragmentSrc The fragment shader source code, can be a file path or source
     *                    code
     * \param sSrcTyp Shader source code type, if it is a link to a file or source code
     *
     * \return Whether the shader was created, linked and added to the manager correctly
     *         or not.
     */
    bool addShaderProgram(ShaderProgram& shaderProgram, const std::string& name,
        const std::string& vertexSrc, const std::string& fragmentSrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::ShaderSourceType::File);

    /**
     * Add a shader program to the manager. The shaders will be compiled and linked to the
     * program. The name of the shader needs to be unique or it won't be added. Both
     * vertex shader and fragment shader source need to be provided, either as a link to a
     * shader source code file or as shader source code.
     *
     * \param name Unique name of the shader
     * \param vertexSrc The vertex shader source code, can be a file path or source code
     * \param fragmentSrc The fragment shader source code, can be a file path or source
     *                    code
     * \param geometrySrc The geometry shader source code, can be a file path or source
     *                    code
     * \param sSrcTyp Shader source code type, if it is a link to a file or source code
     *
     * \return Whether the shader was created, linked and added to the manager correctly
     *         or not.
     */
    bool addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc, const std::string& geometrySrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::ShaderSourceType::File);

    /**
     * Add a shader program to the manager. The shaders will be compiled and linked to the
     * program. The name of the shader needs to be unique or it won't be added. Both
     * vertex shader and fragment shader source need to be provided, either as a link to a
     * shader source code file or as shader source code.
     *
     * \param shaderProgram Reference to ShaderProgram
     * \param name Unique name of the shader
     * \param vertexSrc The vertex shader source code, can be a file path or source code
     * \param fragmentSrc The fragment shader source code, can be a file path or source
     *        code
     * \param geometrySrc The geometry shader source code, can be a file path or source
     *        code
     * \param sSrcTyp Shader source code type, if it is a link to a file or source code
     *
     * \return Whether the shader was created, linked and added to the manager correctly
     *         or not.
     */
    bool addShaderProgram(ShaderProgram& shaderProgram, const std::string& name,
        const std::string& vertexSrc, const std::string& fragmentSrc,
        const std::string& geometrySrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::ShaderSourceType::File);

    /**
     * Reloads a shader program from the manager for the current bin.
     *
     * \param name Name of the shader program to reload
     *
     * \return true if the shader program was reloaded correctly
     */
    bool reloadShaderProgram(const std::string& name);

    /**
     * Removes a shader program from the manager. All resources allocated for the program
     * will be deallocated and removed.
     *
     * \param name Name of the shader program to remove
     *
     * \return true if the shader program was removed correctly
     */
    bool removeShaderProgram(const std::string& name);

    /**
     * Set a shader program to be used in the current rendering pipeline.
     *
     * \param name Name of the shader program to set as active
     *
     * \return Whether the specified shader was set as active or not.
     */
    bool bindShaderProgram(const std::string& name) const;

    /**
     * Set a shader program to be used in the current rendering pipeline.
     *
     * \param shader Reference to the shader program to set as active
     *
     * \return Whether the specified shader was set as active or not.
     */
    bool bindShaderProgram(const ShaderProgram& shaderProgram) const;
    
    /// Unbind/unset/disable current shader program in the rendering pipeline.
    void unBindShaderProgram();

    /**
     * Check if a shader program exists in the manager.
     *
     * \param name Name of the shader program.
     */
    bool shaderProgramExists(const std::string& name) const;
    
    /**
     * Get the specified shader program from the shader manager. If the shader is not
     * found ShaderManager::NullShader will be returned which can be used for comparisons.
     * The NullShader can not be set as active or used in the rendering pipeline.
     *
     * \param name Name of the shader program
     *
     * \return The specified shader program or ShaderManager::NullShader if shader is not
     *         found.
     */
    const ShaderProgram& getShaderProgram(const std::string& name) const;

    /// Get the manager instance
    static ShaderManager* instance();

    /// Destroy the ShaderManager
    static void destroy();

private:
    static ShaderManager* _instance;
    // Active shaders in the manager
    std::vector<ShaderProgram> _shaderPrograms;
};

} // namespace sgct

#endif // __SGCT__SHADER_MANAGER__H__
