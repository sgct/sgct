/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/ShaderManager.h>

#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <algorithm>

namespace sgct {

ShaderManager* ShaderManager::mInstance = nullptr;

ShaderManager* ShaderManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new ShaderManager();
    }

    return mInstance;
}

void ShaderManager::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

ShaderManager::~ShaderManager() {
    for (ShaderProgram& p : mShaderPrograms) {
        p.deleteProgram();
    }
}

bool ShaderManager::addShaderProgram(const std::string& name,
                                     ShaderProgram& shaderProgram)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to add shader program [%s]: Name already exists.\n", name.c_str()
        );
        return false;
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    mShaderPrograms.push_back(sp);
    shaderProgram = mShaderPrograms.back();

    return true;
}

bool ShaderManager::addShaderProgram(const std::string& name,
                                     const std::string& vertexSrc,
                                     const std::string& fragmentSrc,
                                     ShaderProgram::ShaderSourceType sSrcType)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to add shader program [%s]: Name already exists.\n", name.c_str()
        );
        return false;
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    
    if (!sp.addShaderSrc(vertexSrc, GL_VERTEX_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }
    
    if (!sp.addShaderSrc(fragmentSrc, GL_FRAGMENT_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }

    if (sp.createAndLinkProgram()) {
        mShaderPrograms.push_back(sp);
        return true;
    }

    // If arrived here the creation and linking of the program didn't work.
    // Return false but printing errors is handled in createAndLinkProgram()
    return false;
}

bool ShaderManager::addShaderProgram(ShaderProgram& shaderProgram,
                                     const std::string& name,
                                     const std::string& vertexSrc,
                                     const std::string& fragmentSrc,
                                     ShaderProgram::ShaderSourceType sSrcType)
{
    // if something failes set shader pointer to NullShader
    shaderProgram = NullShader;
    
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to add shader program [%s]: Name already exists.\n", name.c_str()
        );
        return false;
    }

    ShaderProgram sp(name);
    
    if (!sp.addShaderSrc(vertexSrc, GL_VERTEX_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }
    
    if (!sp.addShaderSrc(fragmentSrc, GL_FRAGMENT_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }

    if (sp.createAndLinkProgram()) {
        mShaderPrograms.push_back(sp);
        shaderProgram = mShaderPrograms.back();
        return true;
    }

    // If arrived here the creation and linking of the program didn't work.
    // Return false but printing errors is handled in createAndLinkProgram()
    return false;
}

bool ShaderManager::addShaderProgram(const std::string& name,
                                     const std::string& vertexSrc,
                                     const std::string& fragmentSrc,
                                     const std::string& geometrySrc,
                                     ShaderProgram::ShaderSourceType sSrcType)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to add shader program [%s]: Name already exists.\n", name.c_str()
        );
        return false;
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    
    if (!sp.addShaderSrc(vertexSrc, GL_VERTEX_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }
    
    if (!sp.addShaderSrc(fragmentSrc, GL_FRAGMENT_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }

    if (!sp.addShaderSrc(geometrySrc, GL_GEOMETRY_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }

    if (sp.createAndLinkProgram()) {
        mShaderPrograms.push_back(sp);
        return true;
    }

    // If arrived here the creation and linking of the program didn't work.
    // Return false but printing errors is handled in createAndLinkProgram()
    return false;
}

bool ShaderManager::addShaderProgram(ShaderProgram& shaderProgram,
                                     const std::string& name,
                                     const std::string& vertexSrc,
                                     const std::string& fragmentSrc,
                                     const std::string& geometrySrc,
                                     ShaderProgram::ShaderSourceType sSrcType)
{
    //if something failes set shader pointer to NullShader
    shaderProgram = NullShader;
    
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to add shader program [%s]: Name already exists.\n", name.c_str()
        );
        return false;
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    
    if (!sp.addShaderSrc(vertexSrc, GL_VERTEX_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }
    
    if (!sp.addShaderSrc(fragmentSrc, GL_FRAGMENT_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }

    if( !sp.addShaderSrc(geometrySrc, GL_GEOMETRY_SHADER, sSrcType)) {
        // Error messaging handled when setting source
        return false;
    }

    if (sp.createAndLinkProgram()) {
        mShaderPrograms.push_back(sp);
        shaderProgram = mShaderPrograms.back();
        return true;
    }

    // If arrived here the creation and linking of the program didn't work.
    // Return false but printing errors is handled in createAndLinkProgram()
    return false;
}

bool ShaderManager::reloadShaderProgram(const std::string& name) {
    std::vector<ShaderProgram>::iterator shaderIt = std::find_if(
        mShaderPrograms.begin(),
        mShaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );

    if (shaderIt == mShaderPrograms.end()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to reload shader program [%s]: Not found in current bin.\n",
            name.c_str()
        );
        return false;
    }

    shaderIt->reload();

    return true;
}

bool ShaderManager::removeShaderProgram(const std::string& name) {
    std::vector<ShaderProgram>::iterator shaderIt = std::find_if(
        mShaderPrograms.begin(),
        mShaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );

    if (shaderIt == mShaderPrograms.end()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Unable to remove shader program [%s]: Not found in current bin.\n",
            name.c_str()
        );
        return false;
    }

    shaderIt->deleteProgram();
    mShaderPrograms.erase(shaderIt);

    return true;
}

bool ShaderManager::bindShaderProgram(const std::string& name) const {
    const ShaderProgram& sp = getShaderProgram(name);

    if (sp.getName() == NullShader.getName()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Could not set shader program [%s] as active: Not found in manager.\n",
            name.c_str()
        );
        glUseProgram(0); //unbind to prevent errors
        return false;
    }

    return sp.bind();
}

bool ShaderManager::bindShaderProgram(const ShaderProgram& shaderProgram) const {
    if (shaderProgram.getName() == NullShader.getName()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Could not set shader program [Invalid Pointer] as active: Not found in manager.\n"
        );
        glUseProgram(0); //unbind to prevent errors
        return false;
    }

    return shaderProgram.bind();
}

void ShaderManager::unBindShaderProgram() {
    glUseProgram(0);
}

const ShaderProgram& ShaderManager::getShaderProgram(const std::string& name) const {
    std::vector<ShaderProgram>::const_iterator shaderIt = std::find_if(
        mShaderPrograms.begin(),
        mShaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );
    if (shaderIt != mShaderPrograms.end()) {
        return *shaderIt;
    }
    else {
        return NullShader;
    }
}

bool ShaderManager::shaderProgramExists(const std::string& name) const {
    std::vector<ShaderProgram>::const_iterator exists = std::find_if(
        mShaderPrograms.begin(),
        mShaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );

    return exists != mShaderPrograms.end();
}

} // namespace sgct
