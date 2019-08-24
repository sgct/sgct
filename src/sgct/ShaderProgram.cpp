/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/ShaderProgram.h>

#include <sgct/MessageHandler.h>
#include <sgct/ogl_headers.h>

namespace sgct {

ShaderProgram::ShaderProgram(std::string name) : mName(std::move(name)) {}

void ShaderProgram::deleteProgram() {
    for (sgct_core::ShaderData& sd : mShaders) {
        if (sd.mShader.getId() > 0) {
            glDetachShader(mProgramId, sd.mShader.getId());
            sd.mShader.deleteShader();
        }
    }

    glDeleteProgram(mProgramId);
    mProgramId = 0;
}

void ShaderProgram::setName(std::string name) {
    mName = std::move(name);
}

bool ShaderProgram::addShaderSrc(std::string src, sgct_core::Shader::ShaderType type,
                                 ShaderSourceType sSrcType)
{
    sgct_core::ShaderData sd;
    sd.mShader.setShaderType(type);
    sd.mIsSrcFile = sSrcType == ShaderSourceType::File;
    sd.mShaderSrc = std::move(src);

    bool success;
    if (sd.mIsSrcFile) {
        success = sd.mShader.setSourceFromFile(sd.mShaderSrc);
    }
    else {
        success = sd.mShader.setSourceFromString(sd.mShaderSrc);
    }

    if (success) {
        mShaders.push_back(std::move(sd));
    }

    return success;
}

int ShaderProgram::getAttribLocation(const std::string& name) const {
    return glGetAttribLocation(mProgramId, name.c_str());
}

int ShaderProgram::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(mProgramId, name.c_str());
}

void ShaderProgram::bindFragDataLocation(unsigned int colorNumber,
                                         const std::string& name) const
{
    glBindFragDataLocation(mProgramId, colorNumber, name.c_str());
}

std::string ShaderProgram::getName() const {
    return mName;
}

bool ShaderProgram::isLinked() const {
    return mIsLinked;
}

int ShaderProgram::getId() const {
    return mProgramId;
}

bool ShaderProgram::createAndLinkProgram() {
    if (mShaders.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "ShaderProgram: No shaders has been added to program '%s'\n", mName.c_str()
        );
        return false;
    }

    // Create the program
    bool createSuccess = createProgram();
    if (!createSuccess) {
        // Error text handled in createProgram()
        return false;
    }

    // Link shaders
    for (const sgct_core::ShaderData& sd : mShaders) {
        if (sd.mShader.getId() > 0) {
            glAttachShader(mProgramId, sd.mShader.getId());
        }
    }

    glLinkProgram(mProgramId);

    mIsLinked = checkLinkStatus();
    return mIsLinked;
}

bool ShaderProgram::reload() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "ShaderProgram: Reloading program '%s'\n", mName.c_str()
    );
    
    deleteProgram();

    for (sgct_core::ShaderData& sd : mShaders) {
        bool success;
        if (sd.mIsSrcFile) {
            success = sd.mShader.setSourceFromFile(sd.mShaderSrc);
        }
        else {
            success = sd.mShader.setSourceFromString(sd.mShaderSrc);
        }

        if (!success) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "ShaderProgram: Failed to load '%s'\n", sd.mShaderSrc.c_str()
            );
            return false;
        }
    }

    return createAndLinkProgram();
}

bool ShaderProgram::createProgram() {
    if (mProgramId > 0) {
        // If the program is already created don't recreate it.
        // but should only return true if it hasn't been linked yet.
        // if it has been linked already it can't be reused
        if (mIsLinked) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Could not create shader program [%s]: Already linked to shaders\n",
                mName.c_str()
            );
            return false;
        }

        // If the program is already created but not linked yet it can be reused
        return true;
    }

    mProgramId = glCreateProgram();

    if (mProgramId == 0) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Could not create shader program [%s]: Unknown error\n", mName.c_str()
        );
        return false;
    }

    return true;
}

bool ShaderProgram::checkLinkStatus() const {
    GLint linkStatus;
    glGetProgramiv(mProgramId, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE) {
        GLint logLength;
        glGetProgramiv(mProgramId, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<GLchar> log(logLength);
        glGetProgramInfoLog(mProgramId, logLength, nullptr, log.data());

        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Shader program[%s] linking error: %s\n", mName.c_str(), log
        );

        return false;
    }

    return true;
}

bool ShaderProgram::bind() const {
    // Make sure the program is linked before it can be used
    if (!mIsLinked) {
        return false;
    }

    glUseProgram(mProgramId);
    return true;
}

void ShaderProgram::unbind() {
    glUseProgram(0);
}

} // namespace sgct
