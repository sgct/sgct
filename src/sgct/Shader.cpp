/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/Shader.h>

#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <fstream>

namespace sgct_core {

/*!
The constructor sets shader type
  @param shaderType The shader type: GL_COMPUTE_SHADER, GL_VERTEX_SHADER,
                    GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER,
                    or GL_FRAGMENT_SHADER
*/
Shader::Shader(ShaderType shaderType) : mShaderType(shaderType) {}

/*!
Set the shader type
@param shaderType The shader type: GL_COMPUTE_SHADER, GL_VERTEX_SHADER,
                  GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER,
                  or GL_FRAGMENT_SHADER
*/
void Shader::setShaderType(ShaderType shaderType) {
    mShaderType = shaderType;
}

/*!
Set the shader source code from a file, will create and compile the shader if it is not
already done. At this point a compiled shader can't have its source reset. Recompilation
of shaders is not supported
@param file Path to shader file
@return If setting source and compilation went ok.
*/
bool Shader::setSourceFromFile(const std::string& file) {
    // Make sure file can be opened
    std::ifstream shaderFile(file);

    if (!shaderFile.is_open()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Could not open %s file[%s].\n",
            getShaderTypeName(mShaderType).c_str(),
            file.c_str()
        );
        return false;
    }

    // Create needed resources by reading file length
    shaderFile.seekg(0, std::ios_base::end);
    std::streamoff fileSize = shaderFile.tellg();
    shaderFile.seekg(0, std::ios_base::beg);

    // Make sure the file is not empty
    if (fileSize == 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Can't create source for %s: empty file [%s].\n",
            getShaderTypeName(mShaderType).c_str(),
            file.c_str()
        );
        return false;
    }

    // Copy file content to string
    std::vector<char> bytes(fileSize);
    shaderFile.read(bytes.data(), fileSize);
    std::string shaderSrc(bytes.data(), fileSize);
    shaderFile.close();

    // Compile shader source
    return setSourceFromString(shaderSrc);
}

/*!
Set the shader source code from a file, will create and compile the shader if it is not
already done. At this point a compiled shader can't have its source reset. Recompilation
of shaders is not supported
@param sourceString String with shader source code
@return If setting the source and compilation went ok.
*/
bool Shader::setSourceFromString(const std::string& sourceString) {
    // At this point no resetting of shaders are supported
    if (mShaderId > 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "%s is already set for specified shader.\n",
            getShaderTypeName(mShaderType).c_str()
        );
        return false;
    }

    // Prepare source code for shader
    const char* shaderSrc[] = { sourceString.c_str() };

    mShaderId = glCreateShader(mShaderType);
    glShaderSource(mShaderId, 1, shaderSrc, nullptr);

    // Compile and check status
    glCompileShader(mShaderId);

    return checkCompilationStatus();
}

/*! Delete the shader */
void Shader::deleteShader() {
    glDeleteShader(mShaderId);
    mShaderId = 0;
}

int Shader::getId() const {
    return mShaderId;
}

/*!
Will check the compilation status of the shader and output any errors from the shader log
return    Status of the compilation
*/
bool Shader::checkCompilationStatus() const {
    GLint compilationStatus;
    glGetShaderiv(mShaderId, GL_COMPILE_STATUS, &compilationStatus);

    if (compilationStatus == 0) {
        GLint logLength;
        glGetShaderiv(mShaderId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength == 0) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "%s compile error: Unknown error\n",
                getShaderTypeName(mShaderType).c_str()
            );
            return false;
        }

        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(mShaderId, logLength, nullptr, log.data());
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "%s compile error: %s\n",
            getShaderTypeName(mShaderType).c_str(),
            log.data()
        );

        return false;
    }

    return compilationStatus == GL_TRUE;
}

/*!
Will return the name of the shader type
@param shaderType The shader type
@return Shader type name
*/
std::string Shader::getShaderTypeName(ShaderType shaderType) const {
    switch (shaderType) {
        case GL_VERTEX_SHADER:
            return "Vertex shader";
        case GL_FRAGMENT_SHADER:
            return "Fragment shader";
        case GL_GEOMETRY_SHADER:
            return "Geometry shader";
        case GL_COMPUTE_SHADER:
            return "Compute shader";
        case GL_TESS_CONTROL_SHADER:
            return "Tesselation control shader";
        case GL_TESS_EVALUATION_SHADER:
            return "Tesselation evaluation shader";
        default:
            return "Unknown shader";
    };
}

} // namespace sgct_core
