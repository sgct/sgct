/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__FONT_MANAGER__H__
#define __SGCT__FONT_MANAGER__H__

#include <sgct/ShaderProgram.h>
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <unordered_map>

#ifndef SGCT_DONT_USE_EXTERNAL
#include <external/freetype/ftglyph.h>
#include <external/freetype/ftstroke.h>
#else
#include <freetype/ftglyph.h>
#include <freetype/ftstroke.h>
#endif

/*! \namespace sgct_text
\brief SGCT text namespace is used for text rendering and font management
*/
namespace sgct_text {

class Font;

/*!
Singleton for font handling. A lot of the font handling is based on Nehes tutorials for freetype <a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehes tutorials for freetype</a>
\n
\n
How to load a font (somewhere in the openGL init callback or in callbacks with shared openGL context):
\n
\code{.cpp}
//Add Verdana size 14 to the FontManager using the system font path
if( !sgct_text::FontManager::instance()->addFont( "Verdana", "verdana.ttf" ) )
    sgct_text::FontManager::instance()->getFont( "Verdana", 14 );
 
//Add Special font from local path
if( !sgct_text::FontManager::instance()->addFont( "Special", "Special.ttf", sgct_text::FontManager::Local ) )
    sgct_text::FontManager::instance()->getFont( "Special", 14 );
\endcode
\n
Then in the draw or draw2d callback the font can be rendered:
\code{.cpp}
sgct_text::print(sgct_text::FontManager::instance()->getFont( "Verdana", 14 ), sgct_text::TopLeft, 50, 50, "Hello World!");
\endcode
\n
SGCT has an internal font that can be used as well:
\code{.cpp}
sgct_text::print(sgct_text::FontManager::instance()->getDefaultFont( 14 ), sgct_text::TopLeft, 50, 50, "Hello World!");
\endcode
\n
Non ASCII characters are supported as well:
\code{.cpp}
sgct_text::print(sgct_text::FontManager::instance()->getDefaultFont( 14 ), sgct_text::TopLeft, 50, 50, L"Hallå Världen!");
\endcode
*/
class FontManager {
public:
    // Convenience enum from where to load font files
    enum class FontPath { Local, Default };

    ~FontManager();

    bool addFont(std::string fontName, std::string path,
        FontPath fontPath = FontPath::Default);
    Font* getFont(const std::string& name, unsigned int height = mDefaultHeight);
    Font* getDefaultFont(unsigned int height = mDefaultHeight);
    
    void setDefaultFontPath(std::string path);
    void setStrokeColor(glm::vec4 color);
    void setDrawInScreenSpace(bool state);

    size_t getTotalNumberOfLoadedChars();
    glm::vec4 getStrokeColor();
    bool getDrawInScreenSpace();

    const sgct::ShaderProgram& getShader() const;
    unsigned int getMVPLoc();
    unsigned int getColLoc();
    unsigned int getStkLoc();
    unsigned int getTexLoc();

    static FontManager* instance();
    static void destroy();

private:
    FontManager();

    /// Helper functions
    Font* createFont(const std::string& fontName, unsigned int height);

    FontManager(const FontManager & fm) = delete;
    const FontManager& operator=(const FontManager& rhs) = delete;

    static FontManager* mInstance; // Singleton instance of the LogManager
    static const signed short mDefaultHeight = 10; // Default height of font faces in pixels

    // The default font path from where to look for font files
    std::string mDefaultFontPath;

    FT_Library mFTLibrary; // Freetype library
    FT_Face mFace = nullptr;
    glm::vec4 mStrokeColor = glm::vec4(0.f, 0.f, 0.f, 0.9f);

    bool mDrawInScreenSpace = true;

    // Holds all predefined font paths for generating font glyphs
    std::map<std::string, std::string> mFontPaths; 
    // All generated fonts
    std::unordered_map<std::string, std::unordered_map<unsigned int, Font*>> mFontMap;

    sgct::ShaderProgram mShader;
    int mMVPLoc = -1;
    int mColLoc = -1;
    int mStkLoc = -1;
    int mTexLoc = -1;
};

} // sgct

#endif // __SGCT__FONT_MANAGER__H__
