/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/ogl_headers.h>
#include <sgct/Font.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>

/*!
Default constructor for font face data.
*/
sgct_text::FontFaceData::FontFaceData()
{
	mTexId = GL_FALSE;
	mCharWidth = 0.0f;
}

/*!
Default constructor does not allocate any resources for the font.
The init function needs to be called before the font can actually be used
@param	fontName	Name of the font
@param	height		Height of the font
*/
sgct_text::Font::Font( const std::string & fontName, float height ) :
	mName( fontName ),
	mHeight( height ),
	mFontFaceData( NULL ),
	mListBase( GL_FALSE )
{
	mVAO = GL_FALSE;
	mVBO = GL_FALSE;
}

/*!
Destructor does nothing. Fonts should be explicitly called for cleanup (Clean())
*/
sgct_text::Font::~Font()
{
	// Do nothing, need to call Clean explicitly to clean up resources
}

/*!
Initializes all variables needed for the font. Needs to be called
before creating any textures for the font
@param	name	FontName of the font that's being created
@aram	height	Font height in pixels
*/
void sgct_text::Font::init( const std::string & name, unsigned int height )
{
	//Allocate some memory to store the texture ids.
	mName = name;
	mFontFaceData = new FontFaceData[NUM_OF_GLYPHS_TO_LOAD];
	mHeight = static_cast<float>( height );

	if( sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		mListBase = glGenLists(NUM_OF_GLYPHS_TO_LOAD);
	}
	else
	{
		glGenVertexArrays(1, &mVAO);
		glGenBuffers(1, &mVBO);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating VAO: %d\n", mVAO);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating VBO: %d\n", mVBO);
	}
}

void sgct_text::Font::generateTexture(std::size_t c, int width, int height, unsigned char * data, bool generateMipMaps)
{
	unsigned int tex = mFontFaceData[c].mTexId;
	if (tex == GL_FALSE)
	{
		glGenTextures(1, &tex);
		mFontFaceData[static_cast<size_t>(c)].mTexId = tex;
	}

	glBindTexture(GL_TEXTURE_2D, tex);

	/*
	SGCT2 change: Use non-power-of-two textures for better quality
	*/
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (sgct::Engine::instance()->isOGLPipelineFixed())
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_LUMINANCE_ALPHA, width, height,
			0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RG, width, height,
			0, GL_RG, GL_UNSIGNED_BYTE, data);

	if (generateMipMaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
		glGenerateMipmap(GL_TEXTURE_2D); //allocate the mipmaps
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

/*!
Counts the number of textures used by this font.
*/
std::size_t sgct_text::Font::getNumberOfTextures()
{
	std::size_t counter = 0;
	for (std::size_t i = 0; i < NUM_OF_GLYPHS_TO_LOAD; ++i)
		if (mFontFaceData[i].mTexId != GL_FALSE)
			counter++;
	return counter;
}

/*!
Cleans up memory used by the Font
*/
void sgct_text::Font::clean()
{
	if(mFontFaceData)	// Check if init has been called
	{
		if( sgct::Engine::instance()->isOGLPipelineFixed() && mListBase != 0)
			glDeleteLists( mListBase, NUM_OF_GLYPHS_TO_LOAD);
		else
		{
			if(mVAO != 0)
				glDeleteVertexArrays(1, &mVAO);
			if( mVBO != 0)
				glDeleteBuffers(1, &mVBO);
		}

		for (std::size_t i = 0; i < NUM_OF_GLYPHS_TO_LOAD; i++)
			glDeleteTextures( 1, &(mFontFaceData[i].mTexId) );
		delete [] mFontFaceData;
		mFontFaceData = NULL;
	}

	std::vector<FT_Glyph>::iterator it = mGlyphs.begin();
	std::vector<FT_Glyph>::iterator end = mGlyphs.end();

	for( ; it != end; ++it )
	{
		FT_Done_Glyph( *it );
	}

	mGlyphs.clear();
}
