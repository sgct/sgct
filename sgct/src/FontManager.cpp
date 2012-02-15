#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "../include/sgct/FontManager.h"
#include "../include/sgct/MessageHandler.h"

#include <freetype/ftglyph.h>

#include <algorithm>
#include <stdio.h>

using namespace sgct;

//----------------------------------------------------------------------
// Helper functions
//----------------------------------------------------------------------

/*!
This function gets the first power of 2 >= the int that we pass it.
*/
inline int NextP2 ( int a )
{
	int rval=1;
	while(rval<a) rval<<=1;
	return rval;
}

//----------------------------------------------------------------------
// FontManager Implementations
//----------------------------------------------------------------------

/*! Initiate FontManager instance to NULL */
FontManager * FontManager::mInstance = NULL;

/*! Default height in pixels for all font faces */
const FT_Short FontManager::mDefaultHeight = 10;

/*!
Constructor initiates the freetyp library
*/
FontManager::FontManager(void)
{
	FT_Error error = FT_Init_FreeType( &mFTLibrary );

	if ( error != 0 )
	{
		sgct::MessageHandler::Instance()->print("Could not initiate Freetype library.\n" );
		return; // No need to continue
	}

	//
	// Set default font path
	//
	char winDir[128];
	GetWindowsDirectory(winDir,128);
	mDefaultFontPath.assign( winDir );
	mDefaultFontPath += "\\Fonts\\";
}

/*!
Destructor cleans up all font objects
*/
FontManager::~FontManager(void)
{
	std::set<Freetype::Font>::iterator it = mFonts.begin();
	std::set<Freetype::Font>::iterator end = mFonts.end();

	for( ; it != end; ++it )
	{
		const_cast<Freetype::Font&>( (*it) ).clean();
	}

	if( mFTLibrary != NULL )
	{
		FT_Done_FreeType( mFTLibrary );
	}
}

/*!
Set the default font path. This will be the directory where font files will be searched
for by default. If not explicitly set the default font path will be the windows font folder.
@param	path	The directory where the default font files are located
*/
void FontManager::SetDefaultFontPath( const std::string & path )
{
	mDefaultFontPath = path;
}

/*!
Adds a font file to the manager.
@param	fontName	Specify a name for the font
@param	path		Path to the font file
@param	fontPath	If it is a local font path directory or using the default path
*/
bool FontManager::AddFont( const std::string & fontName, std::string path, FontPath fontPath )
{
	// Perform file exists check
	if( fontPath == FontPath_Default )
	{
		path = mDefaultFontPath + path;
	}

	bool inserted = mFontPaths.insert( std::pair<std::string, std::string>( fontName, path ) ).second;

	if( !inserted )
	{

		sgct::MessageHandler::Instance()->print("Font with name '%s' already specified.\n", fontName.c_str() );
		return false;
	}

	return true;
}

/*!
Get a font face that is loaded into memory.
@param	name	Name of the font
@param	height	Height in  pixels for the font
@return	Pointer to the font face, NULL if not found
*/
const Freetype::Font * FontManager::GetFont( const std::string & fontName, unsigned int height )
{
	// If there will be a lot of switching between font sizes consider saving every font face as a unique font instead
	// of resizing
	Freetype::Font searchFont( fontName, static_cast<float>( height ) );

	std::set<Freetype::Font>::iterator it = std::find( mFonts.begin(), mFonts.end(), searchFont );

	if( it == mFonts.end() )
	{
		it = CreateFont( fontName, height );
	}

	return (it != mFonts.end() ) ? &(*it) : NULL;
}

/*!
Creates font textures with a specific height if a path to the font exists
@param	fontName	Name of the font
@param	height		Height of the font in pixels
@return	Iterator to the newly created font, end of the Fonts container if something went wrong
*/
std::set<Freetype::Font>::iterator FontManager::CreateFont( const std::string & fontName, unsigned int height )
{
	std::map<std::string, std::string>::iterator it = mFontPaths.find( fontName );

	if( it == mFontPaths.end() )
	{
		sgct::MessageHandler::Instance()->print("No font file specified for font [%s].\n", fontName.c_str() );
		return mFonts.end();
	}

	if( mFTLibrary == NULL )
	{
		sgct::MessageHandler::Instance()->print("Freetype library is not initialized, can't create font [%s].\n", fontName.c_str() );
		return mFonts.end();
	}

	FT_Face face;
	FT_Error error = FT_New_Face( mFTLibrary, it->second.c_str(), 0, &face );

	if ( error == FT_Err_Unknown_File_Format )
	{
		sgct::MessageHandler::Instance()->print("Unsopperted file format [%s] for font [%s].\n", it->second.c_str(), fontName.c_str() );
		return mFonts.end();
	}
	else if( error != 0 || face == NULL )
	{
		// Implement error message "Unable to read font file [%s]"
		return mFonts.end();
	}

	if( FT_Set_Char_Size( face, height << 6, height << 6, 96, 96) != 0 )
	{
		sgct::MessageHandler::Instance()->print("Could not set pixel size for font[%s].\n", fontName.c_str() );
		return mFonts.end();
	}

	// Create the font when all error tests are done
	Freetype::Font newFont = Freetype::Font();
	newFont.init( fontName, height );


	//This is where we actually create each of the fonts display lists.
	for( unsigned char i = 0;i < 128; ++i )
	{
		if( !MakeDisplayList( face, i, newFont ) )
		{
			newFont.clean();
			return mFonts.end();
		}
	}

	FT_Done_Face(face);

	return mFonts.insert( newFont ).first;
}


/*!
Create a display list for the passed character
@param	face		Font face to create glyph from
@param	ch			Character to create glyph for
@param	font		Font to create for
@param	texBase		Texture base
@return If display list character created successfully
*/
bool FontManager::MakeDisplayList ( FT_Face face, char ch, Freetype::Font & font )
{

	//The first thing we do is get FreeType to render our character
	//into a bitmap.  This actually requires a couple of FreeType commands:

	//Load the Glyph for our character.
	if( FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT ) )
	{
		sgct::MessageHandler::Instance()->print("FT_Load_Glyph failed for char [%c].\n", ch );
		// Implement error message " char %s"
		return false;
	}


	//Move the face's glyph into a Glyph object.
    FT_Glyph glyph;
    if( FT_Get_Glyph( face->glyph, &glyph ) )
	{
		sgct::MessageHandler::Instance()->print("FT_Get_Glyph failed for char [%c].\n", ch );
		return false;
	}

	//Convert the glyph to a bitmap.
	FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

	//This reference will make accessing the bitmap easier
	FT_Bitmap& bitmap = bitmap_glyph->bitmap;

	//Use our helper function to get the widths of
	//the bitmap data that we will need in order to create
	//our texture.
	int width = NextP2( bitmap.width );
	int height = NextP2( bitmap.rows );

	//Allocate memory for the texture data.
	GLubyte* expanded_data = new GLubyte[ 2 * width * height];

	//Here we fill in the data for the expanded bitmap.
	//Notice that we are using two channel bitmap (one for
	//luminocity and one for alpha), but we assign
	//both luminocity and alpha to the value that we
	//find in the FreeType bitmap.
	//We use the ?: operator so that value which we use
	//will be 0 if we are in the padding zone, and whatever
	//is the the Freetype bitmap otherwise.
	for( int j = 0; j < height; ++j )
	{
		for( int i = 0; i < width; ++i )
		{
			expanded_data[2*(i+j*width)]= expanded_data[2*(i+j*width)+1] =
				(i>=bitmap.width || j>=bitmap.rows) ?
				0 : bitmap.buffer[i + bitmap.width*j];
		}
	}


	//Now we just setup some texture paramaters.
	GLuint textureId = font.getTextures()[static_cast<unsigned int>(ch)];
	glBindTexture( GL_TEXTURE_2D, textureId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	//Here we actually create the texture itself, notice
	//that we are using GL_LUMINANCE_ALPHA to indicate that
	//we are using 2 channel data.
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		  0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, expanded_data );

	//With the texture created, we don't need to expanded data anymore
    delete[] expanded_data;

	//So now we can create the display list
	glNewList( font.getListBase() + ch, GL_COMPILE );

	glBindTexture( GL_TEXTURE_2D, textureId );

	glPushMatrix();

	//first we need to move over a little so that
	//the character has the right amount of space
	//between it and the one before it.
	glTranslatef( (GLfloat)bitmap_glyph->left,0,0);

	//Now we move down a little in the case that the
	//bitmap extends past the bottom of the line
	//(this is only true for characters like 'g' or 'y'.
	glTranslatef( 0, (GLfloat)bitmap_glyph->top-bitmap.rows, 0 );

	//Now we need to account for the fact that many of
	//our textures are filled with empty padding space.
	//We figure what portion of the texture is used by
	//the actual character and store that information in
	//the x and y variables, then when we draw the
	//quad, we will only reference the parts of the texture
	//that we contain the character itself.
	float	x=(float)bitmap.width / (float)width,
			y=(float)bitmap.rows / (float)height;

	//Here we draw the texturemaped quads.
	//The bitmap that we got from FreeType was not
	//oriented quite like we would like it to be,
	//so we need to link the texture to the quad
	//so that the result will be properly aligned.

	glBegin(GL_QUADS);
		glTexCoord2d( 0, 0 ); glVertex2f(				   0.0f, (GLfloat)bitmap.rows );
		glTexCoord2d( 0, y ); glVertex2f(				   0.0f,				 0.0f );
		glTexCoord2d( x, y ); glVertex2f( (GLfloat)bitmap.width,				 0.0f );
		glTexCoord2d( x, 0 ); glVertex2f( (GLfloat)bitmap.width, (GLfloat)bitmap.rows );
	glEnd();
	glPopMatrix();

	glTranslatef( (GLfloat)(face->glyph->advance.x >> 6), 0, 0 );


	//increment the raster position as if we were a bitmap font.
	//(only needed if you want to calculate text length)
	glBitmap( 0, 0, 0.0f, 0.0f, (GLfloat)(face->glyph->advance.x >> 6), 0, NULL );

	//Finnish the display list
	glEndList();

	// Can't delete them while they are used, delete when font is cleaned
	font.AddGlyph( glyph );

	return true;
}
