/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/FontManager.h>

#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <stdio.h>

namespace {
    constexpr const char* FontVertShader = R"(**glsl_version**
layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec2 Position;

uniform mat4 MVP;
out vec2 UV;

void main() {
    gl_Position = MVP * vec4(Position, 0.0, 1.0);
    UV = TexCoord;
})";

    constexpr const char* FontFragShader = R"(**glsl_version**
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

    constexpr const char* FontVertShaderLegacy = R"(**glsl_version**

void main() {
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
})";

    constexpr const char* FontFragShaderLegacy = R"(**glsl_version**

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
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

/*!
Constructor initiates the freetyp library
*/
FontManager::FontManager() {
    FT_Error error = FT_Init_FreeType(&mFTLibrary);

    if (error != 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Could not initiate Freetype library.\n"
        );
        return; // No need to continue
    }

    //
    // Set default font path
    //
#if __WIN32__
    char fontDir[128];
    if (GetWindowsDirectory(fontDir, 128) > 0) {
        mDefaultFontPath.assign(fontDir);
        mDefaultFontPath += "\\Fonts\\";
    }
#elif __APPLE__
    //System Fonts
    mDefaultFontPath.assign("/Library/Fonts/");
#else
    mDefaultFontPath.assign("/usr/share/fonts/truetype/freefont/");
#endif
}

/*!
Destructor cleans up all font objects, textures and shaders
*/
FontManager::~FontManager() {
    using K = std::string;
    using V = std::unordered_map<unsigned int, Font*>;
    for (std::pair<const K, V>& a : mFontMap) {
        for (std::pair<const unsigned int, Font*>& b : a.second) {
            b.second->clean();
            delete b.second;
        }
        a.second.clear();
    }
    mFontMap.clear();

    if (mFTLibrary != nullptr) {
        FT_Done_FreeType(mFTLibrary);
    }

    mShader.deleteProgram();
}

/*!
Set the default font path. This will be the directory where font files will be searched
for by default. If not explicitly set the default font path will be the windows font folder.
@param    path    The directory where the default font files are located
*/
void FontManager::setDefaultFontPath(std::string path) {
    mDefaultFontPath = std::move(path);
}

/*!
Set the stroke (border) color
*/
void FontManager::setStrokeColor(glm::vec4 color) {
    mStrokeColor = std::move(color);
}

/*!
Set if screen space coordinates should be used or buffer coordinates
*/
void FontManager::setDrawInScreenSpace(bool state) {
    mDrawInScreenSpace = state;
}

size_t FontManager::getTotalNumberOfLoadedChars() {
    size_t counter = 0;
    using K = std::string;
    using V = std::unordered_map<unsigned int, Font*>;
    for (const std::pair<const K, V>& a : mFontMap) {
        for (const std::pair<const unsigned int, Font*>& b : a.second) {
            counter += b.second->getNumberOfLoadedChars();
        }
    }
    return counter;
}

glm::vec4 FontManager::getStrokeColor() {
    return mStrokeColor;
}

bool FontManager::getDrawInScreenSpace() {
    return mDrawInScreenSpace;
}

sgct::ShaderProgram FontManager::getShader() {
    return mShader;
}

unsigned int FontManager::getMVPLoc() {
    return mMVPLoc;
}

unsigned int FontManager::getColLoc() {
    return mColLoc;
}

unsigned int FontManager::getStkLoc() {
    return mStkLoc;
}

unsigned int FontManager::getTexLoc() {
    return mTexLoc;
}

/*!
Adds a font file to the manager.
@param    fontName    Specify a name for the font
@param    path        Path to the font file
@param    fontPath    If it is a local font path directory or using the default path
*/
bool FontManager::addFont(std::string fontName, std::string path, FontPath fontPath) {
    // Perform file exists check
    if (fontPath == FontPath_Default) {
        path = mDefaultFontPath + path;
    }

    bool inserted = mFontPaths.insert({ std::move(fontName), std::move(path) }).second;

    if (!inserted) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Font with name '%s' already specified.\n", fontName.c_str()
        );
        return false;
    }

    return true;
}

/*!
Get a font face that is loaded into memory.
@param    name    Name of the font
@param    height    Height in  pixels for the font
@return    Pointer to the font face, NULL if not found
*/
Font* FontManager::getFont(const std::string& fontName, unsigned int height) {
    if (mFontMap[fontName].count(height) == 0) {
        mFontMap[fontName][height] = createFont(fontName, height);
    }

    return mFontMap[fontName][height];
}

/*!
Get the SGCT default font face that is loaded into memory.
@param    height    Height in  pixels for the font
@return    Pointer to the font face, NULL if not found
*/
Font * FontManager::getDefaultFont(unsigned int height)
{
    return getFont("SGCTFont", height);
}

/*!
Creates font textures with a specific height if a path to the font exists
@param    fontName    Name of the font
@param    height        Height of the font in pixels
@return    Iterator to the newly created font, end of the Fonts container if something went wrong
*/
Font* FontManager::createFont(const std::string& fontName, unsigned int height) {
    std::map<std::string, std::string>::iterator it = mFontPaths.find(fontName);

    if (it == mFontPaths.end()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: No font file specified for font [%s].\n", fontName.c_str()
        );
        return mFontMap.end()->second.end()->second;
    }

    if (mFTLibrary == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Freetype library is not initialized, can't create font [%s].\n",
            fontName.c_str()
        );
        return mFontMap.end()->second.end()->second;
    }

    FT_Error error = FT_New_Face(mFTLibrary, it->second.c_str(), 0, &mFace);

    if (error == FT_Err_Unknown_File_Format) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Unsopperted file format [%s] for font [%s].\n",
            it->second.c_str(), fontName.c_str()
        );
        return mFontMap.end()->second.end()->second;
    }
    else if (error != 0 || mFace == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Font '%s' not found!\n", it->second.c_str()
        );
        return mFontMap.end()->second.end()->second;
    }

    if (FT_Set_Char_Size(mFace, height << 6, height << 6, 96, 96) != 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "FontManager: Could not set pixel size for font[%s].\n", fontName.c_str()
        );
        return mFontMap.end()->second.end()->second;
    }

    // Create the font when all error tests are done
    Font* newFont = new Font();
    newFont->init(mFTLibrary, mFace, fontName, height);

    static bool shaderCreated = false;

    if (!shaderCreated) {
        std::string vert_shader;
        std::string frag_shader;

        mShader.setName("FontShader");
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            vert_shader = FontVertShaderLegacy;
            frag_shader = FontFragShaderLegacy;
        }
        else {
            vert_shader = FontVertShader;
            frag_shader = FontFragShader;
        }

        //replace glsl version
        sgct_helpers::findAndReplace(
            vert_shader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            frag_shader,
            "**glsl_version**",
            sgct::Engine::instance()->getGLSLVersion()
        );

        bool vertShader = mShader.addShaderSrc(
            vert_shader,
            GL_VERTEX_SHADER,
            sgct::ShaderProgram::SHADER_SRC_STRING
        );
        if (!vertShader) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load font vertex shader\n"
            );
        }
        bool fragShader = mShader.addShaderSrc(
            frag_shader,
            GL_FRAGMENT_SHADER,
            sgct::ShaderProgram::SHADER_SRC_STRING
        );
        if (!fragShader) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Failed to load font fragment shader\n"
            );
        }
        mShader.createAndLinkProgram();
        mShader.bind();

        if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
            mMVPLoc = mShader.getUniformLocation("MVP");
        }
        mColLoc = mShader.getUniformLocation("Col");
        mStkLoc = mShader.getUniformLocation("StrokeCol");
        mTexLoc = mShader.getUniformLocation("Tex");
        mShader.unbind();

        shaderCreated = true;
    }

    //sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Number of textures loaded: %u\n", newFont.getNumberOfTextures());

    mFontMap[fontName][height] = newFont;
    return newFont;
}

} // namespace sgct_text
