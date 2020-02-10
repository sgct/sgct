/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _FREETYPE_FONT_H_
#define _FREETYPE_FONT_H_

#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/freetype/ftglyph.h>
    #include <external/freetype/ftstroke.h>
#else
    #include <freetype/ftglyph.h>
    #include <freetype/ftstroke.h>
#endif

#include <vector>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include "helpers/SGCTCPPEleven.h"

namespace sgct_text
{
class FontFaceData
{
public:
    FontFaceData();
    unsigned int mTexId;
    float mDistToNextChar;
	glm::vec2 mPos = glm::vec2(0.f);
	glm::vec2 mSize = glm::vec2(0.f);
	FT_Glyph mGlyph;
	bool mInterpolated;
};

class GlyphData
{
public:
	FT_Glyph mGlyph;
	FT_Glyph mStrokeGlyph;
	FT_Stroker mStroker;
	FT_BitmapGlyph mBitmapGlyph;
	FT_BitmapGlyph mBitmapStrokeGlyph;
	FT_Bitmap * mBitmapPtr;
	FT_Bitmap * mStrokeBitmapPtr;
};

/*!
Will ahandle font textures and rendering. Implementation is based on
<a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehe's font tutorial for freetype</a>.
*/
class Font
{
public:
    Font( const std::string & fontName = std::string(), float height = 0.0f );
    ~Font();

	void init(FT_Library lib, FT_Face face, const std::string & fontName, unsigned int h );
	std::size_t getNumberOfLoadedChars();
	void clean();

	/*! Get the font face data */
	FontFaceData * getFontFaceData(wchar_t c);

    /*! Get the vertex array id */
    inline unsigned int getVAO() const { return mVAO; }

    /*! Get the vertex buffer objects id */
    inline unsigned int getVBO() const { return mVBO; }

	/*! Get the display list id */
	inline unsigned int getDisplayList() const { return mListId; }

    /*! Get height of the font */
    inline float getHeight() const { return mHeight; }

	const signed long getStrokeSize() const;
	void setStrokeSize(signed long size);

public:

    /*! Less then Font comparison operator */
    inline bool operator<( const Font & rhs ) const
    { return mName.compare( rhs.mName ) < 0 || (mName.compare( rhs.mName ) == 0 && mHeight < rhs.mHeight); }

    /*! Equal to Font comparison operator */
    inline bool operator==( const Font & rhs ) const
    { return mName.compare( rhs.mName ) == 0 && mHeight == rhs.mHeight; }

private:
	void createCharacter(wchar_t c);
	bool createGlyph(wchar_t c, FontFaceData * FFDPtr);
	unsigned int generateTexture(int width, int height, unsigned char * data);

	bool getPixelData(FT_Face face, int & width, int & height, unsigned char ** pixels, GlyphData * gd);
	
    std::string mName;                // Holds the font name
    float mHeight;                    // Holds the height of the font.
	unsigned int mListId;
	unsigned int mVBO;
    unsigned int mVAO;
	FT_Face	mFace;
	FT_Library mFTLibrary;
	FT_Fixed mStrokeSize;
	sgct_cppxeleven::unordered_map<wchar_t, FontFaceData> mFontFaceDataMap;
};

} // sgct

#endif
