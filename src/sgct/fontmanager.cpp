/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

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
    constexpr const char* FontVertShader = R"(
**glsl_version**
layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec2 Position;

uniform mat4 MVP;
out vec2 UV;

void main() {
    gl_Position = MVP * vec4(Position, 0.0, 1.0);
    UV = TexCoord;
})";

    constexpr const char* FontFragShader = R"(
**glsl_version**
uniform vec4 Col;
uniform vec4 StrokeCol;
uniform sampler2D Tex;

in vec2 UV;
out vec4 Color;

void main() {
    vec2 LuminanceAlpha = texture(Tex, UV.st).rg;
    vec4 blend = mix(StrokeCol, Col, LuminanceAlpha.r);
    Color = blend * vec4(1.0, 1.0, 1.0, LuminanceAlpha.g);
})";

    constexpr const char* FontVertShaderLegacy = R"(
**glsl_version**

void main() {
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
})";

    constexpr const char* FontFragShaderLegacy = R"(
**glsl_version**

uniform vec4 Col;
uniform vec4 StrokeCol;
uniform sampler2D Tex;

void main() {
    vec4 LuminanceAlpha = texture2D(Tex, gl_TexCoord[0].st);
    vec4 blend = mix(StrokeCol, Col, LuminanceAlpha.r);
    gl_FragColor = blend * vec4(1.0, 1.0, 1.0, LuminanceAlpha.a);
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
        MessageHandler::instance()->printError("Could not initiate Freetype library");
        return;
    }

    // Set default font path
#ifdef WIN32
    char FontDir[128];
    const UINT success = GetWindowsDirectory(FontDir, 128);
    if (success > 0) {
        _defaultFontPath = FontDir;
        _defaultFontPath += "\\Fonts\\";
    }
#elif defined(__APPLE__)
    //System Fonts
    _defaultFontPath = "/Library/Fonts/";
#else
    _defaultFontPath = "/usr/share/fonts/truetype/freefont/";
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

void FontManager::setDefaultFontPath(std::string path) {
    _defaultFontPath = std::move(path);
}

void FontManager::setStrokeColor(glm::vec4 color) {
    _strokeColor = std::move(color);
}

glm::vec4 FontManager::getStrokeColor() const {
    return _strokeColor;
}

const ShaderProgram& FontManager::getShader() const {
    return _shader;
}

unsigned int FontManager::getMVPLocation() const {
    return _mvpLocation;
}

unsigned int FontManager::getColorLocation() const {
    return _colorLocation;
}

unsigned int FontManager::getStrokeLocation() const {
    return _strokeLocation;
}

unsigned int FontManager::getTextureLoc() const {
    return _textureLocation;
}

bool FontManager::addFont(std::string fontName, std::string path, FontPath fontPath) {
    // Perform file exists check
    if (fontPath == FontPath::Default) {
        path = _defaultFontPath + path;
    }

    bool inserted = _fontPaths.insert({ std::move(fontName), std::move(path) }).second;

    if (!inserted) {
        MessageHandler::instance()->printWarning(
            "Font with name '%s' already specified", fontName.c_str()
        );
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
        MessageHandler::instance()->printError(
            "FontManager: No font file specified for font [%s]", name.c_str()
        );
        return nullptr;
    }

    if (_library == nullptr) {
        MessageHandler::instance()->printError(
            "FontManager: Freetype library is not initialized, can't create font [%s]",
            name.c_str()
        );
        return nullptr;
    }

    FT_Face face;
    FT_Error error = FT_New_Face(_library, it->second.c_str(), 0, &face);

    if (error == FT_Err_Unknown_File_Format) {
        MessageHandler::instance()->printError(
            "FontManager: Unsopperted file format [%s] for font [%s]",
            it->second.c_str(), name.c_str()
        );
        return nullptr;
    }
    else if (error != 0 || face == nullptr) {
        MessageHandler::instance()->printError(
            "FontManager: Font '%s' not found!", it->second.c_str()
        );
        return nullptr;
    }

    FT_Error charSizeErr = FT_Set_Char_Size(face, height << 6, height << 6, 96, 96);
    if (charSizeErr != 0) {
        MessageHandler::instance()->printError(
            "FontManager: Could not set pixel size for font[%s]", name.c_str()
        );
        return nullptr;
    }

    // Create the font when all error tests are done
    std::unique_ptr<Font> font = std::make_unique<Font>(_library, face, name, height);

    static bool shaderCreated = false;

    if (!shaderCreated) {
        _shader.setName("FontShader");
        std::string vertShader = FontVertShader;
        std::string fragShader = FontFragShader;

        // replace glsl version
        helpers::findAndReplace(
            vertShader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        helpers::findAndReplace(
            fragShader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );

        const bool vert = _shader.addShaderSrc(
            vertShader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!vert) {
            MessageHandler::instance()->printError("Failed to load font vertex shader");
        }
        const bool frag = _shader.addShaderSrc(
            fragShader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!frag) {
            MessageHandler::instance()->printError("Failed to load font fragment shader");
        }
        _shader.createAndLinkProgram();
        _shader.bind();

        _mvpLocation = _shader.getUniformLocation("MVP");
        _colorLocation = _shader.getUniformLocation("Col");
        _strokeLocation = _shader.getUniformLocation("StrokeCol");
        _textureLocation = _shader.getUniformLocation("Tex");
        _shader.unbind();

        shaderCreated = true;
    }

    return font;
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
