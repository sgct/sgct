/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/fontmanager.h>

#include <sgct/engine.h>
#include <sgct/font.h>
#include <sgct/messagehandler.h>
#include <sgct/helpers/stringfunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

namespace {
    std::string SystemFontPath;

    constexpr const char* FontVertShader = R"(
#version 330 core
layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec2 Position;

uniform mat4 mvp;
out vec2 uv;

void main() {
    gl_Position = mvp * vec4(Position, 0.0, 1.0);
    uv = TexCoord;
})";

    constexpr const char* FontFragShader = R"(
#version 330 core
uniform vec4 col;
uniform vec4 strokeCol;
uniform sampler2D tex;

in vec2 uv;
out vec4 Color;

void main() {
    vec2 luminanceAlpha = texture(tex, uv.st).rg;
    vec4 blend = mix(strokeCol, col, luminanceAlpha.r);
    Color = blend * vec4(1.0, 1.0, 1.0, luminanceAlpha.g);
})";
} // namespace

namespace sgct::text {

FontManager* FontManager::_instance = nullptr;

FontManager* FontManager::instance() {
    if (_instance == nullptr) {
        _instance = new FontManager();
    }

    return _instance;
}

void FontManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

FontManager::FontManager() {
    FT_Error error = FT_Init_FreeType(&_library);

    if (error != 0) {
        MessageHandler::printError("Could not initiate Freetype library");
        return;
    }

    // Set default font path
#ifdef WIN32
    constexpr const int BufferSize = 256;
    char FontDir[256];
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
    // We need to delete all of the fonts before destroying the FreeType library or else
    // the destructor of the Font classes will access the library after it has been
    // destroyed
    _fontMap.clear();

    if (_library) {
        FT_Done_FreeType(_library);
    }

    _shader.deleteProgram();
}

void FontManager::bindShader(const glm::mat4& mvp, const glm::vec4& color, 
                             const glm::vec4& strokeColor, int texture) const
{
    _shader.bind();

    glUniform4fv(_colorLocation, 1, glm::value_ptr(color));
    glUniform4fv(_strokeLocation, 1, glm::value_ptr(strokeColor));
    glUniform1i(_textureLocation, texture);
    glUniformMatrix4fv(_mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
}

bool FontManager::addFont(std::string name, std::string file, Path path) {
    // Perform file exists check
    if (path == Path::System) {
        file = SystemFontPath + file;
    }

    const bool inserted = _fontPaths.insert({ std::move(name), std::move(file) }).second;
    if (!inserted) {
        MessageHandler::printWarning("Font with name '%s' already exists", name.c_str());
    }
    return inserted;
}

Font* FontManager::getFont(const std::string& fontName, unsigned int height) {
    if (_fontMap.count({ fontName, height }) == 0) {
        std::unique_ptr<Font> f = createFont(fontName, height);
        if (f == nullptr) {
            return nullptr;
        }
        _fontMap[{ fontName, height }] = std::move(f);
    }

    return _fontMap[{ fontName, height }].get();
}

Font* FontManager::getDefaultFont(unsigned int height) {
    return getFont("SGCTFont", height);
}

std::unique_ptr<Font> FontManager::createFont(const std::string& name,
                                              unsigned int height)
{
    std::map<std::string, std::string>::const_iterator it = _fontPaths.find(name);

    if (it == _fontPaths.end()) {
        MessageHandler::printError(
            "FontManager: No font file specified for font [%s]", name.c_str()
        );
        return nullptr;
    }

    if (_library == nullptr) {
        MessageHandler::printError(
            "FontManager: Freetype library is not initialized, can't create font [%s]",
            name.c_str()
        );
        return nullptr;
    }

    FT_Face face;
    FT_Error error = FT_New_Face(_library, it->second.c_str(), 0, &face);

    if (error == FT_Err_Unknown_File_Format) {
        MessageHandler::printError(
            "FontManager: Unsupperted file format [%s] for font [%s]",
            it->second.c_str(), name.c_str()
        );
        return nullptr;
    }
    else if (error != 0 || face == nullptr) {
        MessageHandler::printError(
            "FontManager: Font '%s' not found!", it->second.c_str()
        );
        return nullptr;
    }

    FT_Error charSizeErr = FT_Set_Char_Size(face, height << 6, height << 6, 96, 96);
    if (charSizeErr != 0) {
        MessageHandler::printError(
            "FontManager: Could not set pixel size for font[%s]", name.c_str()
        );
        return nullptr;
    }

    // Create the font when all error tests are done
    std::unique_ptr<Font> font = std::make_unique<Font>(_library, face, height);

    static bool isShaderCreated = false;
    if (!isShaderCreated) {
        _shader.setName("FontShader");
        _shader.addShaderSource(FontVertShader, FontFragShader);
        _shader.createAndLinkProgram();
        _shader.bind();

        _mvpLocation = _shader.getUniformLocation("mvp");
        _colorLocation = _shader.getUniformLocation("col");
        _strokeLocation = _shader.getUniformLocation("strokeCol");
        _textureLocation = _shader.getUniformLocation("tex");
        _shader.unbind();

        isShaderCreated = true;
    }

    return font;
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
