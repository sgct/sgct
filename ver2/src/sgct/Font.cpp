/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/Font.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/MessageHandler.h"

using namespace sgct_text;

/*!
Default constructor does not allocate any resources for the font.
The init function needs to be called before the font can actually be used
@param	fontName	Name of the font
@param	height		Height of the font
*/
Font::Font( const std::string & fontName, float height ) :
	mName( fontName ),
	mHeight( height ),
	mTextures( NULL ),
	mListBase( GL_FALSE ),
	mFixedPipeline( true )
{
	mVAO = GL_FALSE;
	mVBO = GL_FALSE;
	mCharWidths = NULL;
}

/*!
Destructor does nothing. Fonts should be explicitly called for cleanup (Clean())
*/
Font::~Font()
{
	// Do nothing, need to call Clean explicitly to clean up resources
}

/*!
Initializes all variables needed for the font. Needs to be called
before creating any textures for the font
@param	name	FontName of the font that's being created
@aram	height	Font height in pixels
*/
void Font::init( const std::string & name, unsigned int height )
{
	//Allocate some memory to store the texture ids.
	mName = name;
	mTextures = new GLuint[128];
	mCharWidths = new float[128];
	mHeight = static_cast<float>( height );

	glGenTextures( 128, mTextures );
	if( sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		mListBase = glGenLists(128);
	}
	else
	{
		glGenVertexArrays(1, &mVAO);
		glGenBuffers(1, &mVBO);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating VAO: %d\n", mVAO);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating VBO: %d\n", mVBO);
	}
}

/*!
Cleans up memory used by the Font
*/
void Font::clean()
{
	if( mTextures )	// Check if init has been called
	{
		if( sgct::Engine::instance()->isOGLPipelineFixed() && mListBase != 0)
			glDeleteLists( mListBase, 128 );
		else
		{
			if(mVAO != 0)
				glDeleteVertexArrays(1, &mVAO);
			if( mVBO != 0)
				glDeleteBuffers(1, &mVBO);
		}
		glDeleteTextures( 128, mTextures );
		delete [] mTextures;
		mTextures = NULL;
	}

	if( mCharWidths )
	{
		delete [] mCharWidths;
		mCharWidths = NULL;
	}

	std::vector<FT_Glyph>::iterator it = mGlyphs.begin();
	std::vector<FT_Glyph>::iterator end = mGlyphs.end();

	for( ; it != end; ++it )
	{
		FT_Done_Glyph( *it );
	}

	mGlyphs.clear();
}
