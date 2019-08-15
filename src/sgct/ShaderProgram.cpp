/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/ShaderProgram.h>

#include <sgct/MessageHandler.h>
#include <sgct/ogl_headers.h>

namespace sgct {

/*!
Default only sets the program name.Shaders objects won't be created until
the any shader source code is set. The program will be created when the
createAndLink() function is called. Make sure the shader sources are
set before calling it.
@param name Name of the shader program. Must be unique
*/
ShaderProgram::ShaderProgram(std::string name) : mName(std::move(name)) {}

/*!
Will deattach all attached shaders, delete them and then delete the program
*/
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

/*!
@param name Name of the shader program
*/
void ShaderProgram::setName(std::string name) {
    mName = std::move(name);
}

/*!
Will add a shader to the program
@param src Where the source is found, can be either a file path or shader source string
@param type Type of shader can be one of the following: GL_COMPUTE_SHADER,
            GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER,
            GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER
@param sSrcType What type of source code should be read, file or string
@return Whether the source code was set correctly or not
*/
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

/*!
Get the location of the attribute, no explicit error checks are performed.
Users are responsible of checking the return value of the attribute location
@param    name Name of the attribute
@return    Uniform location within the program, -1 if not an active attribute
*/
int ShaderProgram::getAttribLocation(const std::string& name) const {
    return glGetAttribLocation(mProgramId, name.c_str());
}

/*!
Get the location of the attribute, no explicit error checks are performed.
Users are responsible of checking the return value of the attribute location
@param    name Name of the uniform
@return    Uniform location within the program, -1 if not an active uniform
*/
int ShaderProgram::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(mProgramId, name.c_str());
}

/*!
Wrapper for glBindFragDataLocation
bind a user-defined varying out variable to a fragment shader color number
@param    colorNumber The color number to bind the user-defined varying out variable to
@param    name The name of the user-defined varying out variable whose binding to modify
*/
void ShaderProgram::bindFragDataLocation(unsigned int colorNumber,
                                         const std::string& name) const
{
    glBindFragDataLocation(mProgramId, colorNumber, name.c_str());
}

/*! Get the name of the program */
std::string ShaderProgram::getName() const {
    return mName;
}

/*! Check if the program is linked */
bool ShaderProgram::isLinked() const {
    return mIsLinked;
}

/*! Get the program ID */
int ShaderProgram::getId() const {
    return mProgramId;
}

/*!
Will create the program and link the shaders. The shader sources must have been set before
the program can be linked. After the program is created and linked no modification to the
shader sources can be made.
@return    Wheter the program was created and linked correctly or not
*/
bool ShaderProgram::createAndLinkProgram() {
    if (mShaders.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "ShaderProgram: No shaders has been added to program '%s'!\n", mName.c_str()
        );
        return false;
    }

    //
    // Create the program
    //
    bool createSuccess = createProgram();
    if (!createSuccess) {
        // Error text handled in createProgram()
        return false;
    }

    //
    // Link shaders
    //
    for (const sgct_core::ShaderData& sd : mShaders) {
        if (sd.mShader.getId() > 0) {
            glAttachShader(mProgramId, sd.mShader.getId());
        }
    }

    glLinkProgram(mProgramId);

    mIsLinked = checkLinkStatus();
    return mIsLinked;
}

/*!
Reloads a shader by deleting, recompiling and re-linking.
@return Whether the program was created and linked correctly or not
*/
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
                "ShaderProgram: Failed to load '%s'!\n", sd.mShaderSrc.c_str()
            );
            return false;
        }
    }

    return createAndLinkProgram();
}

/*!
Will create the program.
@return Wheter the program was properly created or not
*/
bool ShaderProgram::createProgram() {
    if (mProgramId > 0) {
        // If the program is already created don't recreate it.
        // but should only return true if it hasn't been linked yet.
        // if it has been linked already it can't be reused
        if (mIsLinked) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Could not create shader program [%s]: Already linked to shaders.\n",
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
            "Could not create shader program [%s]: Unknown error.\n", mName.c_str()
        );
        return false;
    }

    return true;
}

/*!
Will check the link status of the program and output any errors from the program log
@return Status of the compilation
*/
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

/*!
Use the shader program in the current rendering pipeline
*/
bool ShaderProgram::bind() const {
    //
    // Make sure the program is linked before it can be used
    //
    if (!mIsLinked) {
        return false;
    }

    glUseProgram(mProgramId);
    return true;
}

/*!
Unset the shader program in the current rendering pipeline
*/
void ShaderProgram::unbind() {
    glUseProgram(0);
}

} // namespace sgct
