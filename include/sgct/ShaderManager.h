/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SHADER_MANAGER__H__
#define __SGCT__SHADER_MANAGER__H__

#include <sgct/ShaderProgram.h>
#include <string>
#include <vector>

namespace sgct {

/*!
For managing shader programs. Implemented as a singleton
The current implementation of shader programs only support vertex and fragment shaders.
*/
class ShaderManager {
public:
    // A shader program that never will be initialized.
    // Will be returned for not found programs and can be used as 
    // comparison for NULL values
    ShaderProgram NullShader = ShaderProgram("SGCT_NULL");

    ~ShaderManager();

    bool addShaderProgram(const std::string& name, ShaderProgram& shaderProgram);

    bool addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc,
        ShaderProgram::ShaderSourceType sSrcType =
            ShaderProgram::ShaderSourceType::File);

    bool addShaderProgram(ShaderProgram& shaderProgram, const std::string& name,
        const std::string& vertexSrc, const std::string& fragmentSrc,
        ShaderProgram::ShaderSourceType sSrcType =
            ShaderProgram::ShaderSourceType::File);

    bool addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc, const std::string& geometrySrc,
        ShaderProgram::ShaderSourceType sSrcType =
            ShaderProgram::ShaderSourceType::File);

    bool addShaderProgram(ShaderProgram& shaderProgram, const std::string& name,
        const std::string& vertexSrc, const std::string& fragmentSrc,
        const std::string& geometrySrc,
        ShaderProgram::ShaderSourceType sSrcType =
            ShaderProgram::ShaderSourceType::File);

    bool reloadShaderProgram(const std::string& name);
    bool removeShaderProgram(const std::string& name);
    bool bindShaderProgram(const std::string& name) const;
    bool bindShaderProgram(const ShaderProgram& shaderProgram) const;
    void unBindShaderProgram();

    bool shaderProgramExists(const std::string& name) const;
    
    const ShaderProgram& getShaderProgram(const std::string& name) const;

    /*! Get the manager instance */
    static ShaderManager* instance();

    /*! Destroy the ShaderManager */
    static void destroy();

private:
    static ShaderManager* mInstance;
    // Active shaders in the manager
    std::vector<ShaderProgram> mShaderPrograms;
};

} // namespace sgct

#endif // __SGCT__SHADER_MANAGER__H__
