/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FONTMANAGER__H__
#define __SGCT__FONTMANAGER__H__

#ifdef SGCT_HAS_TEXT

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <sgct/shaderprogram.h>

#include <map>
#include <memory>
#include <string>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif // __clang__

#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

typedef struct FT_LibraryRec_  *FT_Library;

namespace sgct::text {

class Font;

/**
 * Singleton for font handling. A lot of the font handling is based on Nehes tutorials for
 * freetype [link](http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/")
 *
 * How to load a font (somewhere in the OpenGL init callback or in callbacks with shared
 * OpenGL context):
 *
 * ```cpp
 * //Add Verdana size 14 to the FontManager using the system font path
 * if (!sgct::text::FontManager::instance().addFont("Verdana", "verdana.ttf"))
 *    sgct::text::FontManager::instance().getFont("Verdana", 14);
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
 * ```
 *
 * Then in the draw or draw2d callback the font can be rendered:
 * ```cpp
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getFont("Verdana", 14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     "Hello World"
 * );
 * ```
 *
 * SGCT has an internal font that can be used as well:
 * ```cpp
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     "Hello World"
 * );
 * ```
 *
 * Non ASCII characters are supported as well:
 * ```cpp
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     L"Hallå Världen"
 * );
 * ```
 */
class SGCT_EXPORT FontManager {
public:
    static FontManager& instance();
    static void destroy();

    /**
     * Destructor cleans up all font objects, textures and shaders.
     */
    ~FontManager();

    /**
     * Adds a font file to the manager.
     *
     * \param name Specify a name for the font
     * \param file Path to the font file
     */
    bool addFont(std::string name, std::string file);

    /**
     * Get a font face that is loaded into memory.
     *
     * \param name Name of the font
     * \param height Height in  pixels for the font
     * \return Pointer to the font face, `nullptr` if not found
     */
    Font* font(const std::string& name, unsigned int height = 10);

    /**
     * Binds the font shader and also sets the four uniform values for the
     * modelviewprojectionmatrix, the inner color of the text, the stroke color, and the
     * texture index at which the font information is stored.
     */
    void bindShader(const mat4& mvp, const vec4& color, int texture) const;

private:
    /**
     * Constructor initiates the freetype library.
     */
    FontManager();

    /**
     * Creates font textures with a specific height if a path to the font exists.
     *
     * \param fontName Name of the font
     * \param height Height of the font in pixels
     * \return Pointer to the newly created font, nullptr if something went wrong
     */
    std::unique_ptr<Font> createFont(const std::string& fontName, int height);

    static FontManager* _instance;

    FT_Library _library;

    /// Holds all predefined font paths for generating font glyphs
    std::map<std::string, std::string> _fontPaths;

    /// All generated fonts
    std::map<std::pair<std::string, unsigned int>, std::unique_ptr<Font>> _fontMap;

    ShaderProgram _shader;
    int _mvpLocation = -1;
    int _colorLocation = -1;
    int _textureLocation = -1;
};

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
#endif // __SGCT__FONTMANAGER__H__
