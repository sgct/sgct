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

#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace sgct_text {

/*!
Will handle font textures and rendering. Implementation is based on
<a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehe's font tutorial for freetype</a>.
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

    Font(std::string fontName = std::string(), float height = 0.f);

    void init(FT_Library lib, FT_Face face, std::string fontName, unsigned int h);
    size_t getNumberOfLoadedChars();
    void clean();

    /*! Get the font face data */
    FontFaceData* getFontFaceData(wchar_t c);

    /*! Get the vertex array id */
    unsigned int getVAO() const;

    /*! Get the vertex buffer objects id */
    unsigned int getVBO() const;

    /*! Get the display list id */
    unsigned int getDisplayList() const;

    /*! Get height of the font */
    float getHeight() const;

    signed long getStrokeSize() const;
    void setStrokeSize(signed long size);

    /*! Less then Font comparison operator */
    bool operator<(const Font& rhs) const;

    /*! Equal to Font comparison operator */
    bool operator==(const Font& rhs) const;

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
    unsigned int generateTexture(int width, int height, unsigned char* data);

    bool getPixelData(FT_Face face, int& width, int& height, unsigned char** pixels,
        GlyphData& gd);
    
    std::string mName; // Holds the font name
    float mHeight; // Holds the height of the font.
    unsigned int mListId = 0;
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
    FT_Face	mFace;
    FT_Library mFTLibrary;
    FT_Fixed mStrokeSize;
    std::unordered_map<wchar_t, FontFaceData> mFontFaceDataMap;
};

} // sgct

#endif // __SGCT__FREETYPE_FONT__H__
