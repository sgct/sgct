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

void ShaderManager::addShaderProgram(const std::string& name,
                                        const std::string& vertexSrc,
                                        const std::string& fragmentSrc,
                                        const std::string& geometrySrc)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        throw std::runtime_error(
            "Unable to add shader program [" + name + "]: Name already exists"
        );
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(name);
    sp.addShaderSource(vertexSrc, GL_VERTEX_SHADER);
    sp.addShaderSource(fragmentSrc, GL_FRAGMENT_SHADER);
    if (!geometrySrc.empty()) {
        sp.addShaderSource(geometrySrc, GL_GEOMETRY_SHADER);
    }

    sp.createAndLinkProgram();
    _shaderPrograms.push_back(std::move(sp));
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
        throw std::runtime_error("Could not find shader with name " + name);
    }
}

bool ShaderManager::shaderProgramExists(const std::string& name) const {
    std::vector<ShaderProgram>::const_iterator exists = std::find_if(
        _shaderPrograms.cbegin(),
        _shaderPrograms.cend(),
        [name](const ShaderProgram& prg) { return prg.getName() == name; }
    );

    return exists != _shaderPrograms.cend();
}

} // namespace sgct
