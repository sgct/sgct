/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/fontmanager.h>

#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <sstream>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // __clang__

#include <freetype/ftglyph.h>

#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__
#pragma GCC diagnostic pop
#endif // __clang__

namespace {
    std::string SystemFontPath;

    constexpr std::string_view FontVertShader = R"(
#version 460 core

layout (location = 0) in vec2 in_texCoords;
layout (location = 1) in vec2 in_position;

out Data {
  vec2 texCoords;
} out_data;

uniform mat4 mvp;


void main() {
  gl_Position = mvp * vec4(in_position, 0.0, 1.0);
  out_data.texCoords = in_texCoords;
})";

    constexpr std::string_view FontFragShader = R"(
#version 460 core

in Data {
  vec2 texCoords;
} in_data;

out vec4 out_color;

uniform vec4 col;
uniform sampler2D tex;


const vec4 StrokeCol = vec4(0.0, 0.0, 0.0, 0.9);

void main() {
    vec2 luminanceAlpha = texture(tex, in_data.texCoords).rg;
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
    const FT_Error error = FT_Init_FreeType(&_library);

    if (error != 0) {
        Log::Error("Could not initiate Freetype library");
        return;
    }

    // Set default font path
#ifdef WIN32
#ifdef UNICODE
    constexpr int BufferSize = MAX_PATH;
    WCHAR winDir[BufferSize];
    const UINT success = GetWindowsDirectoryW(winDir, BufferSize);
    if (success > 0) {
        std::wstringstream ss;
        ss << winDir << "\\Fonts\\";
        std::wstring wSystemFontPath = ss.str();
        SystemFontPath = std::string(wSystemFontPath.begin(), wSystemFontPath.end());
    }
#else // !UNICODE
    constexpr int BufferSize = 256;
    char winDir[BufferSize];
    const UINT success = GetWindowsDirectoryA(winDir, BufferSize);
    if (success > 0) {
        std::stringstream ss;
        ss << winDir << "\\Fonts\\";
        std::string wSystemFontPath = ss.str();
        SystemFontPath = std::string(wSystemFontPath.begin(), wSystemFontPath.end());
    }
#endif // UNICODE
#elif defined(__APPLE__)
    // System Fonts
    SystemFontPath = "/System/Library/Fonts/";
#else // !WIN32 && !__APPLE__
    SystemFontPath = "/usr/share/fonts/truetype/freefont/";
#endif // WIN32
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

void FontManager::bindShader(const mat4& mvp, const vec4& color, int texture) const {
    _shader.bind();

    glProgramUniform4fv(_shader.id(), _colorLocation, 1, &color.x);
    glProgramUniform1i(_shader.id(), _textureLocation, texture);
    glProgramUniformMatrix4fv(_shader.id(), _mvpLocation, 1, GL_FALSE, mvp.values.data());
}

bool FontManager::addFont(std::string name, std::string file, bool isAbsolutePath) {
    // Perform file exists check
    if (!isAbsolutePath) {
        file = SystemFontPath + file;
    }

    const bool inserted = _fontPaths.insert({ name, std::move(file) }).second;
    if (!inserted) {
        Log::Warning(std::format("Font with name '{}' already exists", name));
    }
    return inserted;
}

Font* FontManager::font(const std::string& name, unsigned int height) {
    if (!_fontMap.contains({ name, height })) {
        std::unique_ptr<Font> f = createFont(name, height);
        if (!f) {
            return nullptr;
        }
        _fontMap[{ name, height }] = std::move(f);
    }

    return _fontMap[{ name, height }].get();
}

std::unique_ptr<Font> FontManager::createFont(const std::string& name, int height) {
    const auto it = _fontPaths.find(name);

    if (it == _fontPaths.end()) {
        Log::Error(std::format("No font file specified for font '{}'", name));
        return nullptr;
    }

    if (_library == nullptr) {
        Log::Error(std::format(
            "Freetype library is not initialized, cannot create font '{}'", name
        ));
        return nullptr;
    }

    FT_Face face = nullptr;
    const FT_Error error = FT_New_Face(_library, it->second.c_str(), 0, &face);

    if (error == FT_Err_Unknown_File_Format) {
        Log::Error(std::format(
            "Unsupported file format '{}' for font '{}'", it->second, name
        ));
        return nullptr;
    }
    else if (error != 0 || face == nullptr) {
        Log::Error(std::format("Font '{}' not found", it->second));
        return nullptr;
    }

    const FT_Error charSizeErr = FT_Set_Char_Size(face, height << 6, height << 6, 96, 96);
    if (charSizeErr != 0) {
        Log::Error(std::format("Could not set pixel size for font '{}'", name));
        return nullptr;
    }

    // Create the font when all error tests are done
    auto font = std::make_unique<Font>(_library, face, height);

    static bool isShaderCreated = false;
    if (!isShaderCreated) {
        _shader = ShaderProgram("FontShader");
        _shader.addVertexShader(FontVertShader);
        _shader.addFragmentShader(FontFragShader);
        _shader.createAndLinkProgram();

        _mvpLocation = glGetUniformLocation(_shader.id(), "mvp");
        _colorLocation = glGetUniformLocation(_shader.id(), "col");
        _textureLocation = glGetUniformLocation(_shader.id(), "tex");

        isShaderCreated = true;
    }

    return font;
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
