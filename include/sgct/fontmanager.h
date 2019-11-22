/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__FONT_MANAGER__H__
#define __SGCT__FONT_MANAGER__H__

#ifdef SGCT_HAS_TEXT

#include <sgct/shaderprogram.h>
#include <freetype/ftglyph.h>
#include <freetype/ftstroke.h>
#include <glm/fwd.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

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
 * if (!sgct::text::FontManager::instance().addFont( "Verdana", "verdana.ttf"))
 *    sgct::text::FontManager::instance().getFont( "Verdana", 14 );
 *
 * //Add Special font from local path
 * if (!sgct::text::FontManager::instance().addFont(
 *       "Special",
 *       "Special.ttf",
 *       sgct::text::FontManager::Local
 *  ))
 * {
 *   sgct::text::FontManager::instance().getFont("Special", 14);
 * }
 * \endcode
 *
 * Then in the draw or draw2d callback the font can be rendered:
 * \code{.cpp}
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getFont("Verdana", 14),
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
 *     sgct::text::FontManager::instance().getDefaultFont(14),
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
 *     sgct::text::FontManager::instance().getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     L"Hallå Världen!"
 * );
 * \endcode
 */
class FontManager {
public:
    /// Enum from where to load font files
    enum class Path { System, Local };

    static FontManager& instance();
    static void destroy();

    /// Destructor cleans up all font objects, textures and shaders
    ~FontManager();

    /**
     * Adds a font file to the manager.
     *
     * \param name Specify a name for the font
     * \param file Path to the font file
     * \param path If it is a local font path directory or using the default path
     */
    bool addFont(std::string name, std::string file, Path path = Path::System);

    /**
     * Get a font face that is loaded into memory.
     *
     * \param name Name of the font
     * \param height Height in  pixels for the font
     * \return Pointer to the font face, NULL if not found
     */
    Font* getFont(const std::string& name, unsigned int height = 10);

    /**
     * Get the SGCT default font face that is loaded into memory.
     *
     * \param height Height in  pixels for the font
     * \return Pointer to the font face, nullptr if not found
     */
    Font* getDefaultFont(unsigned int height = 10);
    
    /**
     * Binds the font shader and also sets the four uniform values for the
     * modelviewprojectionmatrix, the inner color of the text, the stroke color, and the
     * texture index at which the font information is stored
     */
    void bindShader(const glm::mat4& mvp, const glm::vec4& color, 
        const glm::vec4& strokeColor, int texture) const;

private:
    /// Constructor initiates the freetype library
    FontManager();

    /**
     * Creates font textures with a specific height if a path to the font exists.
     *
     * \param fontName Name of the font
     * \param height Height of the font in pixels
     * \return Pointer to the newly created font, nullptr if something went wrong
     */
    std::unique_ptr<Font> createFont(const std::string& fontName, unsigned int height);

    static FontManager* _instance;

    FT_Library _library;

    // Holds all predefined font paths for generating font glyphs
    std::map<std::string, std::string> _fontPaths; 

    // All generated fonts
    std::map<std::pair<std::string, unsigned int>, std::unique_ptr<Font>> _fontMap;

    ShaderProgram _shader;
    int _mvpLocation = -1;
    int _colorLocation = -1;
    int _strokeLocation = -1;
    int _textureLocation = -1;
};

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
#endif // __SGCT__FONT_MANAGER__H__
