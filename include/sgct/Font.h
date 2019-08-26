/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__FREETYPE_FONT__H__
#define __SGCT__FREETYPE_FONT__H__

#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/freetype/ftglyph.h>
    #include <external/freetype/ftstroke.h>
#else
    #include <freetype/ftglyph.h>
    #include <freetype/ftstroke.h>
#endif

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace sgct_text {

/**
 * Will handle font textures and rendering. Implementation is based on
 * <a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehe's font
 * tutorial for freetype</a>.
 */
class Font {
public:
    struct FontFaceData {
        unsigned int mTexId = 0;
        float mDistToNextChar = 0.f;
        glm::vec2 mPos;
        glm::vec2 mSize;
        FT_Glyph mGlyph;
        bool mInterpolated = false;
    };

    /**
     * Initializes all variables needed for the font. Needs to be called before creating
     * any textures for the font.
     *
     * \param face The truetype face pointer
     * \param name FontName of the font that's being created
     * \param height Font height in pixels
     */
    Font(FT_Library lib, FT_Face face, std::string fontName, unsigned int h);

    /// Counts the number of textures used by this font.
    size_t getNumberOfLoadedChars() const;
    
    /// Cleans up memory used by the Font
    void clean();

    /// Get the font face data
    const Font::FontFaceData& getFontFaceData(wchar_t c);

    /// Get the vertex array id
    unsigned int getVAO() const;

    /// Get the vertex buffer objects id
    unsigned int getVBO() const;

    /// Get the display list id
    unsigned int getDisplayList() const;

    /// Get height of the font
    float getHeight() const;

    /**
     * Get the stroke (border) size.
     *
     * \return size The stroke size in pixels
     */
    long getStrokeSize() const;

    /**
     * Set the stroke (border) size.
     *
     * \param size The stroke size in pixels
     */
    void setStrokeSize(long size);

private:
    struct GlyphData {
        FT_Glyph mGlyph;
        FT_Glyph mStrokeGlyph;
        FT_Stroker mStroker;
        FT_BitmapGlyph mBitmapGlyph;
        FT_BitmapGlyph mBitmapStrokeGlyph;
        FT_Bitmap* mBitmapPtr;
        FT_Bitmap* mStrokeBitmapPtr;
    };

    void createCharacter(wchar_t c);
    bool createGlyph(wchar_t c, FontFaceData& FFDPtr);
    unsigned int generateTexture(int width, int height,
        const std::vector<unsigned char>& buffer);

    bool getPixelData(FT_Face face, int& width, int& height,
        std::vector<unsigned char>& pixels, GlyphData& gd);
    
    std::string mName;
    float mHeight;
    unsigned int mListId = 0;
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
    FT_Face	mFace;
    FT_Library mFTLibrary;
    FT_Fixed mStrokeSize = 1;
    std::unordered_map<wchar_t, FontFaceData> mFontFaceDataMap;
};

} // sgct

#endif // __SGCT__FREETYPE_FONT__H__
