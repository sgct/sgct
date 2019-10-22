/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/shadermanager.h>

#include <sgct/ogl_headers.h>
#include <sgct/messagehandler.h>
#include <algorithm>

namespace sgct {

ShaderManager* ShaderManager::_instance = nullptr;

ShaderManager* ShaderManager::instance() {
    if (_instance == nullptr) {
        _instance = new ShaderManager();
    }

    return _instance;
}

void ShaderManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

ShaderManager::~ShaderManager() {
    for (ShaderProgram& p : _shaderPrograms) {
        p.deleteProgram();
    }
}

// @TODO (abock, 2019-10-19) Remove this and replace with a function that just adds an
// existing program to the manager
bool ShaderManager::addShaderProgram(const std::string& name,
                                     const std::string& vertexSrc,
                                     const std::string& fragmentSrc)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::printWarning(
            "Unable to add shader program [%s]: Name already exists", name.c_str()
        );
        return false;
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    
    if (!sp.addShaderSource(vertexSrc, GL_VERTEX_SHADER)) {
        // Error messaging handled when setting source
        return false;
    }
    
    if (!sp.addShaderSource(fragmentSrc, GL_FRAGMENT_SHADER)) {
        // Error messaging handled when setting source
        return false;
    }

    if (sp.createAndLinkProgram()) {
        _shaderPrograms.push_back(sp);
        return true;
    }

    // If arrived here the creation and linking of the program didn't work.
    // Return false but printing errors is handled in createAndLinkProgram()
    return false;
}

bool ShaderManager::addShaderProgram(const std::string& name,
                                     const std::string& vertexSrc,
                                     const std::string& fragmentSrc,
                                     const std::string& geometrySrc)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        MessageHandler::printWarning(
            "Unable to add shader program [%s]: Name already exists", name.c_str()
        );
        return false;
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    
    if (!sp.addShaderSource(vertexSrc, GL_VERTEX_SHADER)) {
        // Error messaging handled when setting source
        return false;
    }
    
    if (!sp.addShaderSource(fragmentSrc, GL_FRAGMENT_SHADER)) {
        // Error messaging handled when setting source
        return false;
    }

    if (!sp.addShaderSource(geometrySrc, GL_GEOMETRY_SHADER)) {
        // Error messaging handled when setting source
        return false;
    }

    if (sp.createAndLinkProgram()) {
        _shaderPrograms.push_back(sp);
        return true;
    }

    // If arrived here the creation and linking of the program didn't work.
    // Return false but printing errors is handled in createAndLinkProgram()
    return false;
}

bool ShaderManager::removeShaderProgram(const std::string& name) {
    std::vector<ShaderProgram>::iterator shaderIt = std::find_if(
        _shaderPrograms.begin(),
        _shaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );

    if (shaderIt == _shaderPrograms.end()) {
        MessageHandler::printWarning(
            "Unable to remove shader program [%s]: Not found in current bin", name.c_str()
        );
        return false;
    }

    shaderIt->deleteProgram();
    _shaderPrograms.erase(shaderIt);

    return true;
}

// bool ShaderManager::bindShaderProgram(const std::string& name) const {
//     const ShaderProgram& sp = getShaderProgram(name);

//     if (sp.getName() == NullShader.getName()) {
//         MessageHandler::printWarning(
//             "Could not set shader program [%s] as active: Not found in manager",
//             name.c_str()
//         );
//         glUseProgram(0); //unbind to prevent errors
//         return false;
//     }

//     return sp.bind();
// }

// bool ShaderManager::bindShaderProgram(const ShaderProgram& shaderProgram) const {
//     if (shaderProgram.getName() == NullShader.getName()) {
//         MessageHandler::printWarning(
//             "Could not set shader program [Invalid Pointer] as active"
//         );
//         glUseProgram(0);
//         return false;
//     }

//     return shaderProgram.bind();
// }

// void ShaderManager::unBindShaderProgram() {
//     glUseProgram(0);
// }

const ShaderProgram& ShaderManager::getShaderProgram(const std::string& name) const {
    std::vector<ShaderProgram>::const_iterator shaderIt = std::find_if(
        _shaderPrograms.begin(),
        _shaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );
    if (shaderIt != _shaderPrograms.end()) {
        return *shaderIt;
    }
    else {
        return NullShader;
    }
}

bool ShaderManager::shaderProgramExists(const std::string& name) const {
    std::vector<ShaderProgram>::const_iterator exists = std::find_if(
        _shaderPrograms.begin(),
        _shaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );

    return exists != _shaderPrograms.end();
}

} // namespace sgct
