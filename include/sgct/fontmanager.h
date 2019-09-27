/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__FONT_MANAGER__H__
#define __SGCT__FONT_MANAGER__H__

#ifdef SGCT_HAS_TEXT

#include <sgct/shaderprogram.h>
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include <freetype/ftglyph.h>
#include <freetype/ftstroke.h>

/**
 * \namespace sgct::text
 * \brief SGCT text namespace is used for text rendering and font management
 */
namespace sgct::text {

class Font;

/**
 * Singleton for font handling. A lot of the font handling is based on Nehes tutorials for
 * freetype <a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">
 * Nehes tutorials for freetype</a>
 *
 *
 * How to load a font (somewhere in the OpenGL init callback or in callbacks with shared
 * OpenGL context):
 *
 * \code{.cpp}
 * //Add Verdana size 14 to the FontManager using the system font path
 * if (!sgct::text::FontManager::instance()->addFont( "Verdana", "verdana.ttf"))
 *    sgct::text::FontManager::instance()->getFont( "Verdana", 14 );
 *
 * //Add Special font from local path
 * if (!sgct::text::FontManager::instance()->addFont(
 *       "Special",
 *       "Special.ttf",
 *       sgct::text::FontManager::Local
 *  ))
 * {
 *   sgct::text::FontManager::instance()->getFont("Special", 14);
 * }
 * \endcode
 *
 * Then in the draw or draw2d callback the font can be rendered:
 * \code{.cpp}
 * sgct::text::print(
 *     sgct::text::FontManager::instance()->getFont("Verdana", 14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     "Hello World!"
 * );
 * \endcode
 *
 * SGCT has an internal font that can be used as well:
 * \code{.cpp}
 * sgct::text::print(
 *     sgct::text::FontManager::instance()->getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     "Hello World!"
 * );
 *\endcode
 *
 * Non ASCII characters are supported as well:
 * \code{.cpp}
 * sgct::text::print(
 *     sgct::text::FontManager::instance()->getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     L"Hallå Världen!"
 * );
 * \endcode
 */
class FontManager {
public:
    /// Convenience enum from where to load font files
    enum class FontPath { Local, Default };

    static FontManager* instance();
    static void destroy();

    /// Destructor cleans up all font objects, textures and shaders
    ~FontManager();

    /**
     * Adds a font file to the manager.
     *
     * \param fontName Specify a name for the font
     * \param path Path to the font file
     * \param fontPath If it is a local font path directory or using the default path
     */
    bool addFont(std::string fontName, std::string path,
        FontPath fontPath = FontPath::Default);

    /**
     * Get a font face that is loaded into memory.
     *
     * \param name Name of the font
     * \param height Height in  pixels for the font
     *
     * \return Pointer to the font face, NULL if not found
     */
    Font* getFont(const std::string& name, unsigned int height = 10);

    /**
     * Get the SGCT default font face that is loaded into memory.
     *
     * \param height Height in  pixels for the font
     *
     * \return Pointer to the font face, nullptr if not found
     */
    Font* getDefaultFont(unsigned int height = 10);
    
    /**
     * Set the default font path. This will be the directory where font files will be
     * searched for by default. If not explicitly set the default font path will be the
     * windows font folder.
     *
     * \param path The directory where the default font files are located
     */
    void setDefaultFontPath(std::string path);

    /// Set the stroke (border) color
    void setStrokeColor(glm::vec4 color);

    glm::vec4 getStrokeColor() const;

    const ShaderProgram& getShader() const;
    unsigned int getMVPLocation() const;
    unsigned int getColorLocation() const;
    unsigned int getStrokeLocation() const;
    unsigned int getTextureLoc() const;

private:
    /// Constructor initiates the freetyp library
    FontManager();

    /**
     * Creates font textures with a specific height if a path to the font exists.
     *
     * \param fontName Name of the font
     * \param height Height of the font in pixels
     *
     * \return Pointer to the newly created font, nullptr if something went wrong
     */
    std::unique_ptr<Font> createFont(const std::string& fontName, unsigned int height);

    FontManager(const FontManager & fm) = delete;
    const FontManager& operator=(const FontManager& rhs) = delete;

    static FontManager* mInstance;

    // The default font path from where to look for font files
    std::string mDefaultFontPath;

    FT_Library mFTLibrary;
    glm::vec4 mStrokeColor = glm::vec4(0.f, 0.f, 0.f, 0.9f);

    // Holds all predefined font paths for generating font glyphs
    std::map<std::string, std::string> mFontPaths; 

    // All generated fonts
    std::map<std::pair<std::string, unsigned int>, std::unique_ptr<Font>> mFontMap;

    ShaderProgram mShader;
    int mMVPLocation = -1;
    int mColorLocation = -1;
    int mStrokeLocation = -1;
    int mTextureLocation = -1;
};

} // namespace sgct

#endif // SGCT_HAS_TEXT

#endif // __SGCT__FONT_MANAGER__H__
