/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/shadermanager.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <algorithm>

#define Error(code, msg) Error(Error::Component::Shader, code, msg)

namespace sgct {

ShaderManager* ShaderManager::_instance = nullptr;

ShaderManager& ShaderManager::instance() {
    if (!_instance) {
        _instance = new ShaderManager();
    }
    return *_instance;
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

void ShaderManager::addShaderProgram(std::string name, std::string_view vertexSrc,
                                     std::string_view fragmentSrc)
{
    // Check if shader already exists
    if (shaderProgramExists(name)) {
        throw Error(
            7000,
            fmt::format("Cannot add shader program [{}]: Already exists", name)
        );
    }

    // If shader don't exist, create it and add to container
    ShaderProgram sp(std::move(name));
    sp.addShaderSource(vertexSrc, GL_VERTEX_SHADER);
    sp.addShaderSource(fragmentSrc, GL_FRAGMENT_SHADER);
    sp.createAndLinkProgram();
    _shaderPrograms.push_back(std::move(sp));
}

bool ShaderManager::removeShaderProgram(std::string_view name) {
    const auto shaderIt = std::find_if(
        _shaderPrograms.begin(),
        _shaderPrograms.end(),
        [name](const ShaderProgram& prg) { return prg.name() == name; }
    );

    if (shaderIt == _shaderPrograms.end()) {
        Log::Warning(
            fmt::format("Unable to remove shader program [{}]: Not found", name)
        );
        return false;
    }

    shaderIt->deleteProgram();
    _shaderPrograms.erase(shaderIt);

    return true;
}

const ShaderProgram& ShaderManager::shaderProgram(std::string_view name) const {
    const auto shaderIt = std::find_if(
        _shaderPrograms.cbegin(),
        _shaderPrograms.cend(),
        [name](const ShaderProgram& prg) { return prg.name() == name; }
    );
    if (shaderIt == _shaderPrograms.end()) {
        throw Error(7001, fmt::format("Could not find shader with name {}", name));
    }
    return *shaderIt;
}

bool ShaderManager::shaderProgramExists(std::string_view name) const {
    const auto exists = std::find_if(
        _shaderPrograms.cbegin(),
        _shaderPrograms.cend(),
        [name](const ShaderProgram& prg) { return prg.name() == name; }
    );

    return exists != _shaderPrograms.cend();
}

} // namespace sgct
