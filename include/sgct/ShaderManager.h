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

#define NUMBER_OF_SHADER_BINS 8

namespace sgct {

/*!
For managing shader programs. Implemented as a singleton
The current implementation of shader programs only support vertex and fragment shaders.
*/
class ShaderManager {
public:
    //! Different shader bin indexes to use with the manager
    enum ShaderBinIndex {
        SHADER_BIN_0 = 0,
        SHADER_BIN_1,
        SHADER_BIN_2,
        SHADER_BIN_3,
        SHADER_BIN_4,
        SHADER_BIN_5,
        SHADER_BIN_6,
        SHADER_BIN_7
    };

    // A shader program that never will be initialized.
    // Will be returned for not found programs and can be used as 
    // comparison for NULL values
    ShaderProgram NullShader = ShaderProgram("SGCT_NULL");


    ~ShaderManager();

    bool addShaderProgram(const std::string& name, ShaderProgram& shaderProgram);

    bool addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::SHADER_SRC_FILE );

    bool addShaderProgram(ShaderProgram& shaderProgram, const std::string& name,
        const std::string& vertexSrc, const std::string& fragmentSrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::SHADER_SRC_FILE );

    bool addShaderProgram(const std::string& name, const std::string& vertexSrc,
        const std::string& fragmentSrc, const std::string& geometrySrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::SHADER_SRC_FILE );

    bool addShaderProgram(ShaderProgram& shaderProgram, const std::string& name,
        const std::string& vertexSrc, const std::string& fragmentSrc,
        const std::string& geometrySrc,
        ShaderProgram::ShaderSourceType sSrcType = ShaderProgram::SHADER_SRC_FILE );

    void setCurrentBin(ShaderBinIndex bin );
    bool reloadShaderProgram(const std::string& name );
    bool removeShaderProgram(const std::string& name );
    bool removeShaderProgram(const std::string& name, ShaderBinIndex bin );
    bool bindShaderProgram(const std::string& name ) const;
    bool bindShaderProgram(const ShaderProgram& shaderProgram ) const;
    void unBindShaderProgram();

    bool shaderProgramExists(const std::string& name ) const;
    
    const ShaderProgram& getShaderProgram(const std::string& name) const;

    /*! Get the manager instance */
    static ShaderManager* instance();

    /*! Destroy the ShaderManager */
    static void destroy();

private:
    static ShaderManager* mInstance;
    size_t mCurrentBin = SHADER_BIN_0;
    // Active shaders in the manager
    std::vector<ShaderProgram> mShaderPrograms[NUMBER_OF_SHADER_BINS];
};

} // sgct

#endif // __SGCT__SHADER_MANAGER__H__
