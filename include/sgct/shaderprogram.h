/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SHADERPROGRAM__H__
#define __SGCT__SHADERPROGRAM__H__

#include <sgct/sgctexports.h>
#include <string>
#include <vector>

namespace sgct {

/**
 * Class for handling compiling, linking and using shader programs. Uniform and attribute
 * handling must be managed explicitly.
*/
class SGCT_EXPORT ShaderProgram {
public:
    ShaderProgram() = default;

    /**
     * Default only sets the program name. Shaders objects won't be created until any
     * shader source code is set. The program will be created when the createAndLink()
     * function is called. Make sure the shader sources are set before calling it.
     *
     * \param name Name of the shader program. Must be unique
     */
    ShaderProgram(std::string name);
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) noexcept;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram& operator=(ShaderProgram&&) noexcept;

    /**
     * The destructor clears the shader data vector but the program can still be used. The
     * program have to be destroyed explicitly by calling deleteProgram. This is so that
     * programs can be copied when storing in containers.
     */
    ~ShaderProgram();

    /**
     * Will detach all attached shaders, delete them and then delete the program.
     */
    void deleteProgram();

    /**
     * Will create and add a vertex shader to the program.
     *
     * \param src The shader source string
     * \throw std::runtime_error If the adding of the shaders failed
     */
    void addVertexShader(std::string_view src);

    /**
     * Will create and add a fragment shader to the program.
     *
     * \param src The shader source string
     * \throw std::runtime_error If the adding of the shaders failed
     */
    void addFragmentShader(std::string_view src);

    /**
     * Will create the program and link the shaders. The shader sources must have been set
     * before the program can be linked. After the program is created and linked no
     * modification to the shader sources can be made.
     */
    void createAndLinkProgram();

    /**
     * Use the shader program in the current rendering pipeline.
     */
    void bind() const;

    /**
     * Unset the shader program in the current rendering pipeline.
     */
    static void unbind();

    /**
     * \return The name of the program
     */
    std::string_view name() const;

    /**
     * \return The program ID
     */
    unsigned int id() const;

private:
    /**
     * Will create and the program and return whether it was properly created or not.
     */
    void createProgram();

    /// Name of the program, has to be unique
    std::string _name;
    /// Unique program id
    unsigned int _programId = 0;

    std::vector<unsigned int> _shaders;
};

} // namespace sgct

#endif // __SGCT__SHADERPROGRAM__H__
