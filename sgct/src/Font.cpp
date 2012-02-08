#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "sgct/Font.h"

#define SAFE_DELETE( p ) { if( p != NULL ) { delete p; p = NULL; } };
#define SAFE_DELETE_A( p ) { if( p != NULL ) { delete[] p; p = NULL; } };

using namespace Freetype;

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
	mListBase( 0 )
{
	// Do nothing
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
	mHeight = static_cast<float>( height );

	mListBase = glGenLists(128);
	glGenTextures( 128, mTextures );
}

/*!
Cleans up memory used by the Font
*/
void Font::clean()
{
	if( mTextures )	// Check if init has been called
	{
		glDeleteLists( mListBase, 128 );
		glDeleteTextures( 128, mTextures );
		SAFE_DELETE_A( mTextures );
	}

	std::vector<FT_Glyph>::iterator it = mGlyphs.begin();
	std::vector<FT_Glyph>::iterator end = mGlyphs.end();

	for( ; it != end; ++it )
	{
		FT_Done_Glyph( *it );
	}

	mGlyphs.clear();

}