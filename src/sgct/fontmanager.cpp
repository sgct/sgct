/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/fontmanager.h>

#include <sgct/font.h>
#include <sgct/log.h>
#include <sgct/ogl_headers.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VC_EXTRALEAN
#include <Windows.h>
#endif // WIN32

namespace {
    std::string SystemFontPath;

    constexpr const char* FontVertShader = R"(
#version 330 core
layout (location = 0) in vec2 in_texCoord;
layout (location = 1) in vec2 in_position;
out vec2 tr_uv;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(in_position, 0.0, 1.0);
    tr_uv = in_texCoord;
})";

    constexpr const char* FontFragShader = R"(
#version 330 core
in vec2 tr_uv;
out vec4 out_color;

uniform vec4 col;
uniform sampler2D tex;

const vec4 StrokeCol = vec4(0.0, 0.0, 0.0, 0.9);

void main() {
    vec2 luminanceAlpha = texture(tex, tr_uv).rg;
    vec4 blend = mix(StrokeCol, col, luminanceAlpha.r);
    out_color = blend * vec4(1.0, 1.0, 1.0, luminanceAlpha.g);
})";
} // namespace

namespace sgct::text {

FontManager* FontManager::_instance = nullptr;

FontManager& FontManager::instance() {
    if (!_instance) {
        _instance = new FontManager;
    }
    return *_instance;
}

void FontManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

FontManager::FontManager() {
    FT_Error error = FT_Init_FreeType(&_library);

    if (error != 0) {
        Log::Error("Could not initiate Freetype library");
        return;
    }

    // Set default font path
#ifdef WIN32
    constexpr const int BufferSize = 256;
    char FontDir[BufferSize];
    const UINT success = GetWindowsDirectory(FontDir, 256);
    if (success > 0) {
        SystemFontPath = FontDir;
        SystemFontPath += "\\Fonts\\";
    }
#elif defined(__APPLE__)
    // System Fonts
    SystemFontPath = "/Library/Fonts/";
#else
    SystemFontPath = "/usr/share/fonts/truetype/freefont/";
#endif
}

FontManager::~FontManager() {
    // We need to delete all of the fonts before destroying the FreeType library or the
    // destructor of the Font classes will access the library after it has been destroyed
    _fontMap.clear();

    if (_library) {
        FT_Done_FreeType(_library);
    }

    _shader.deleteProgram();
}

void FontManager::bindShader(const glm::mat4& mvp, const glm::vec4& color,
                             int texture) const
{
    _shader.bind();

    glUniform4fv(_colorLocation, 1, glm::value_ptr(color));
    glUniform1i(_textureLocation, texture);
    glUniformMatrix4fv(_mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
}

bool FontManager::addFont(const std::string& name, std::string file) {
    // Perform file exists check
    file = SystemFontPath + file;

    const bool inserted = _fontPaths.insert({ name, std::move(file) }).second;
    if (!inserted) {
        Log::Warning("Font with name '%s' already exists", name.c_str());
    }
    return inserted;
}

Font* FontManager::font(const std::string& fontName, unsigned int height) {
    if (_fontMap.count({ fontName, height }) == 0) {
        std::unique_ptr<Font> f = createFont(fontName, height);
        if (f == nullptr) {
            return nullptr;
        }
        _fontMap[{ fontName, height }] = std::move(f);
    }

    return _fontMap[{ fontName, height }].get();
}

std::unique_ptr<Font> FontManager::createFont(const std::string& name, int height) {
    std::map<std::string, std::string>::const_iterator it = _fontPaths.find(name);

    if (it == _fontPaths.end()) {
        Log::Error("No font file specified for font [%s]", name.c_str());
        return nullptr;
    }

    if (_library == nullptr) {
        Log::Error(
            "Freetype library is not initialized, can't create font [%s]", name.c_str()
        );
        return nullptr;
    }

    FT_Face face;
    FT_Error error = FT_New_Face(_library, it->second.c_str(), 0, &face);

    if (error == FT_Err_Unknown_File_Format) {
        Log::Error(
            "Unsupperted file format [%s] for font [%s]", it->second.c_str(), name.c_str()
        );
        return nullptr;
    }
    else if (error != 0 || face == nullptr) {
        Log::Error("Font '%s' not found!", it->second.c_str());
        return nullptr;
    }

    FT_Error charSizeErr = FT_Set_Char_Size(face, height << 6, height << 6, 96, 96);
    if (charSizeErr != 0) {
        Log::Error("Could not set pixel size for font[%s]", name.c_str());
        return nullptr;
    }

    // Create the font when all error tests are done
    std::unique_ptr<Font> font = std::make_unique<Font>(_library, face, height);

    static bool isShaderCreated = false;
    if (!isShaderCreated) {
        _shader = ShaderProgram("FontShader");
        _shader.addShaderSource(FontVertShader, FontFragShader);
        _shader.createAndLinkProgram();
        _shader.bind();

        _mvpLocation = glGetUniformLocation(_shader.id(), "mvp");
        _colorLocation = glGetUniformLocation(_shader.id(), "col");
        _textureLocation = glGetUniformLocation(_shader.id(), "tex");
        ShaderProgram::unbind();

        isShaderCreated = true;
    }

    return font;
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
