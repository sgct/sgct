/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/FontManager.h>

#include <sgct/Engine.h>
#include <sgct/Font.h>
#include <sgct/MessageHandler.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

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

namespace sgct_text {

FontManager* FontManager::mInstance = nullptr;

FontManager* FontManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new FontManager();
    }

    return mInstance;
}

void FontManager::destroy() {
    delete mInstance;
    mInstance = nullptr;
}

FontManager::FontManager() {
    FT_Error error = FT_Init_FreeType(&mFTLibrary);

    if (error != 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Could not initiate Freetype library\n"
        );
        return;
    }

    // Set default font path
#ifdef WIN32
    char FontDir[128];
    const UINT success = GetWindowsDirectory(FontDir, 128);
    if (success > 0) {
        mDefaultFontPath = FontDir;
        mDefaultFontPath += "\\Fonts\\";
    }
#elif defined(__APPLE__)
    //System Fonts
    mDefaultFontPath = "/Library/Fonts/";
#else
    mDefaultFontPath = "/usr/share/fonts/truetype/freefont/";
#endif
}

FontManager::~FontManager() {
    // We need to delete all of the fonts before destroying the FreeType library or else
    // the destructor of the Font classes will access the library after it has been
    // destroyed
    mFontMap.clear();

    if (mFTLibrary) {
        FT_Done_FreeType(mFTLibrary);
    }

    mShader.deleteProgram();
}

void FontManager::setDefaultFontPath(std::string path) {
    mDefaultFontPath = std::move(path);
}

void FontManager::setStrokeColor(glm::vec4 color) {
    mStrokeColor = std::move(color);
}

glm::vec4 FontManager::getStrokeColor() const {
    return mStrokeColor;
}

const sgct::ShaderProgram& FontManager::getShader() const {
    return mShader;
}

unsigned int FontManager::getMVPLocation() const {
    return mMVPLocation;
}

unsigned int FontManager::getColorLocation() const {
    return mColorLocation;
}

unsigned int FontManager::getStrokeLocation() const {
    return mStrokeLocation;
}

unsigned int FontManager::getTextureLoc() const {
    return mTextureLocation;
}

bool FontManager::addFont(std::string fontName, std::string path, FontPath fontPath) {
    // Perform file exists check
    if (fontPath == FontPath::Default) {
        path = mDefaultFontPath + path;
    }

    bool inserted = mFontPaths.insert({ std::move(fontName), std::move(path) }).second;

    if (!inserted) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Font with name '%s' already specified.\n", fontName.c_str()
        );
    }

    return inserted;
}

Font* FontManager::getFont(const std::string& fontName, unsigned int height) {
    if (mFontMap.count({ fontName, height }) == 0) {
        std::unique_ptr<Font> f = createFont(fontName, height);
        if (f == nullptr) {
            return nullptr;
        }
        mFontMap[{ fontName, height }] = std::move(f);
    }

    return mFontMap[{ fontName, height }].get();
}

Font* FontManager::getDefaultFont(unsigned int height) {
    return getFont("SGCTFont", height);
}

std::unique_ptr<Font> FontManager::createFont(const std::string& name,
                                              unsigned int height)
{
    std::map<std::string, std::string>::const_iterator it = mFontPaths.find(name);

    if (it == mFontPaths.end()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: No font file specified for font [%s].\n", name.c_str()
        );
        return nullptr;
    }

    if (mFTLibrary == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Freetype library is not initialized, can't create font [%s].\n",
            name.c_str()
        );
        return nullptr;
    }

    FT_Face face;
    FT_Error error = FT_New_Face(mFTLibrary, it->second.c_str(), 0, &face);

    if (error == FT_Err_Unknown_File_Format) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Unsopperted file format [%s] for font [%s].\n",
            it->second.c_str(), name.c_str()
        );
        return nullptr;
    }
    else if (error != 0 || face == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Font '%s' not found!\n", it->second.c_str()
        );
        return nullptr;
    }

    if (FT_Set_Char_Size(face, height << 6, height << 6, 96, 96) != 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Could not set pixel size for font[%s].\n", name.c_str()
        );
        return nullptr;
    }

    // Create the font when all error tests are done
    std::unique_ptr<Font> font = std::make_unique<Font>(mFTLibrary, face, name, height);

    static bool shaderCreated = false;

    if (!shaderCreated) {
        std::string vertShader;
        std::string fragShader;

        mShader.setName("FontShader");
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            vertShader = FontVertShaderLegacy;
            fragShader = FontFragShaderLegacy;
        }
        else {
            vertShader = FontVertShader;
            fragShader = FontFragShader;
        }

        // replace glsl version
        sgct_helpers::findAndReplace(
            vertShader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            fragShader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );

        bool vert = mShader.addShaderSrc(
            vertShader,
            GL_VERTEX_SHADER,
            sgct::ShaderProgram::ShaderSourceType::String
        );
        if (!vert) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load font vertex shader\n"
            );
        }
        bool frag = mShader.addShaderSrc(
            fragShader,
            GL_FRAGMENT_SHADER,
            sgct::ShaderProgram::ShaderSourceType::String
        );
        if (!frag) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load font fragment shader\n"
            );
        }
        mShader.createAndLinkProgram();
        mShader.bind();

        if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
            mMVPLocation = mShader.getUniformLocation("MVP");
        }
        mColorLocation = mShader.getUniformLocation("Col");
        mStrokeLocation = mShader.getUniformLocation("StrokeCol");
        mTextureLocation = mShader.getUniformLocation("Tex");
        mShader.unbind();

        shaderCreated = true;
    }

    return font;
}

} // namespace sgct_text
