/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SHADER_PROGRAM__H__
#define __SGCT__SHADER_PROGRAM__H__

#include <sgct/Shader.h>
#include <sgct/ShaderData.h>
#include <vector>

namespace sgct {

/*!
Helper class for handling compiling, linking and using shader programs. Current
implementation only supports vertex and fragment shader. Uniform and attribute handling
must be managed explicitly but it is possible to poll the Shader program for uniform and
attribute locations.
*/
class ShaderProgram {
public:
    /*! If shader source should be loaded from file or read as is */
    enum class ShaderSourceType { File, String };

    ShaderProgram() = default;
    ShaderProgram(std::string name);

    /*!
    The destructor clears the shader data vector but the program can still be used. The
    program have to be destroyed explicitly by calling deleteProgram. This is so that
    programs can be copied when storing in containers.
    */
    ~ShaderProgram() = default;

    void deleteProgram();

    void setName(std::string name);
    bool addShaderSrc(std::string src, sgct_core::Shader::ShaderType type,
        ShaderSourceType sSrcType = ShaderSourceType::File);

    bool createAndLinkProgram();
    bool reload();

    bool bind() const;
    static void unbind();

    int getAttribLocation(const std::string& name) const;
    int getUniformLocation(const std::string& name) const;
    void bindFragDataLocation(unsigned int colorNumber, const std::string& name) const;

    /*! Get the name of the program */
    std::string getName() const;

    /*! Check if the program is linked */
    bool isLinked() const;

    /*! Get the program ID */
    int getId() const;

private:
    bool createProgram();
    bool checkLinkStatus() const;

    std::string mName = "SGCT_NULL"; /// Name of the program, has to be unique
    bool mIsLinked = false;          /// If this program has been linked
    int mProgramId = 0;              /// Unique program id

    std::vector<sgct_core::ShaderData> mShaders;
};

} // sgct

#endif // __SGCT__SHADER_PROGRAM__H__
