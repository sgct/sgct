/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <GL/glew.h>
#ifdef __WIN32__
#include <GL/wglew.h>
#elif __LINUX__
#include <GL/glext.h>
#else
#include <OpenGL/glext.h>
#endif
#include <GL/glfw.h>
#include "../include/sgct/FontManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/ShaderManager.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <external/freetype/ftglyph.h>

#include <algorithm>
#include <stdio.h>

using namespace sgct_text;

const static std::string Font_Vert_Shader = "\
#version 330 core\n\
\n\
layout (location = 0) in vec2 TexCoord;\n\
layout (location = 1) in vec2 Position;\n\
\n\
uniform mat4 MVP;\n\
out vec2 UV;\n\
\n\
void main()\n\
{\n\
	gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
	UV = TexCoord;\n\
};\n";

const static std::string Font_Frag_Shader = "\
#version 330 core\n\
\n\
uniform vec4 Col;\n\
uniform sampler2D Tex;\n\
\n\
in vec2 UV;\n\
out vec4 Color;\n\
\n\
void main()\n\
{\n\
	vec2 LuminanceAlpha = texture(Tex, UV.st).rg;\n\
	Color.rgb = Col.rgb * LuminanceAlpha.r;\n\
	Color.a = Col.a * LuminanceAlpha.g;\n\
};\n";

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

	char fontDir[128];
#if __WIN32__
	GetWindowsDirectory(fontDir,128);
    mDefaultFontPath.assign( fontDir );
	mDefaultFontPath += "\\Fonts\\";
#elif __APPLE__
	//System Fonts
    sprintf(fontDir, "/Library/Fonts/");
    mDefaultFontPath.assign( fontDir );
#else
    sprintf(fontDir, "/usr/share/fonts/truetype/freefont/");
    mDefaultFontPath.assign( fontDir );
#endif

	mMVPLoc = -1;
	mColLoc = -1;
	mTexLoc = -1;
}

/*!
Destructor cleans up all font objects
*/
FontManager::~FontManager(void)
{
	std::set<Font>::iterator it = mFonts.begin();
	std::set<Font>::iterator end = mFonts.end();

	for( ; it != end; ++it )
	{
		const_cast<Font&>( (*it) ).clean();
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
const Font * FontManager::GetFont( const std::string & fontName, unsigned int height )
{
	// If there will be a lot of switching between font sizes consider saving every font face as a unique font instead
	// of resizing
	Font searchFont( fontName, static_cast<float>( height ) );

	std::set<Font>::iterator it = std::find( mFonts.begin(), mFonts.end(), searchFont );

	if( it == mFonts.end() )
	{
		it = CreateFont( fontName, height );
	}

	return (it != mFonts.end() ) ? &(*it) : NULL;
}

/*!
Get the SGCT default font face that is loaded into memory.
@param	height	Height in  pixels for the font
@return	Pointer to the font face, NULL if not found
*/
const Font * FontManager::GetDefaultFont( unsigned int height )
{
	return GetFont("SGCTFont", height);
}

/*!
Creates font textures with a specific height if a path to the font exists
@param	fontName	Name of the font
@param	height		Height of the font in pixels
@return	Iterator to the newly created font, end of the Fonts container if something went wrong
*/
std::set<Font>::iterator FontManager::CreateFont( const std::string & fontName, unsigned int height )
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
	Font newFont = Font();
	newFont.init( fontName, height );


	//This is where we actually create each of the fonts display lists.
	if(sgct::Engine::Instance()->isOGLPipelineFixed() )
	{
		for( unsigned char i = 0;i < 128; ++i )
			if(!MakeDisplayList( face, i, newFont ))
			{
				newFont.clean();
				return mFonts.end();
			}
	}
	else if(!sgct::Engine::Instance()->isOGLPipelineFixed())
	{
		static bool shaderCreated = false;

		if( !shaderCreated )
		{
			sgct::ShaderManager::Instance()->addShader( mShader, "SGCTFontShader",
			Font_Vert_Shader,
			Font_Frag_Shader, sgct::ShaderManager::SHADER_SRC_STRING );
			mShader.bind();

			mMVPLoc = mShader.getUniformLocation( "MVP" );
			mColLoc = mShader.getUniformLocation( "Col" );
			mTexLoc = mShader.getUniformLocation( "Tex" );
			sgct::ShaderManager::Instance()->unBindShader();

			shaderCreated = true;
		}

		if( !MakeVBO( face, newFont ) )
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
bool FontManager::MakeDisplayList ( FT_Face face, char ch, Font & font )
{

	//The first thing we do is get FreeType to render our character
	//into a bitmap.  This actually requires a couple of FreeType commands:

	//Load the Glyph for our character.
	/*
	 Hints:
	 http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX
	*/
	//if( FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT ) )
	if( FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_FORCE_AUTOHINT ) )
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
	GLuint textureId = font.getTextures()[ static_cast<size_t>(ch) ];
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

	//glPushMatrix();

	//first we need to move over a little so that
	//the character has the right amount of space
	//between it and the one before it.
	//glTranslatef( (GLfloat)bitmap_glyph->left,0,0);

	//Now we move down a little in the case that the
	//bitmap extends past the bottom of the line
	//(this is only true for characters like 'g' or 'y'.
	//glTranslatef( 0, (GLfloat)bitmap_glyph->top-bitmap.rows, 0 );

	glm::vec2 offset( static_cast<float>(bitmap_glyph->left),
			static_cast<float>(bitmap_glyph->top-bitmap.rows));

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
		glTexCoord2f( 0.0f, 0.0f );
		glVertex3f( offset.x, (GLfloat)bitmap.rows + offset.y, 0.0f );

		glTexCoord2f( 0.0f, y );
		glVertex3f(	offset.x, offset.y, 0.0f );

		glTexCoord2f( x, y );
		glVertex3f( (GLfloat)bitmap.width + offset.x, offset.y, 0.0f );

		glTexCoord2f( x, 0.0f );
		glVertex3f( (GLfloat)bitmap.width + offset.x, (GLfloat)bitmap.rows + offset.y, 0.0f );
	glEnd();
	//glPopMatrix();

	//Finnish the display list
	glEndList();

	// Can't delete them while they are used, delete when font is cleaned
	font.AddGlyph( glyph );
	font.setCharWidth( ch, static_cast<float>( face->glyph->advance.x >> 6 ) );

	return true;
}

/*!
Create vertex buffer objects for the passed character
@param	face		Font face to create glyph from
@param	ch			Character to create glyph for
@param	font		Font to create for
@param	texBase		Texture base
@return If display list character created successfully
*/
bool FontManager::MakeVBO( FT_Face face, Font & font )
{
	std::vector<float> coords;

	for( unsigned char ch = 0; ch < 128; ++ch )
	{

		if( FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_FORCE_AUTOHINT ) )
		{
			sgct::MessageHandler::Instance()->print("FT_Load_Glyph failed for char [%c].\n", ch );
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
		GLuint textureId = font.getTextures()[ch];
		glBindTexture( GL_TEXTURE_2D, textureId );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

		//Here we actually create the texture itself, notice
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RG8, width, height,
			  0, GL_RG, GL_UNSIGNED_BYTE, expanded_data );

		//With the texture created, we don't need to expanded data anymore
		delete[] expanded_data;

		glm::vec2 offset( static_cast<float>(bitmap_glyph->left),
			static_cast<float>(bitmap_glyph->top-bitmap.rows));

		//Now we need to account for the fact that many of
		//our textures are filled with empty padding space.
		//We figure what portion of the texture is used by
		//the actual character and store that information in
		//the x and y variables, then when we draw the
		//quad, we will only reference the parts of the texture
		//that we contain the character itself.
		float	x=(float)bitmap.width / (float)width,
				y=(float)bitmap.rows / (float)height;

		coords.push_back( 0.0f );
		coords.push_back( y );
		coords.push_back( offset.x );
		coords.push_back( offset.y );

		coords.push_back( x );
		coords.push_back( y );
		coords.push_back( static_cast<float>(bitmap.width) + offset.x );
		coords.push_back( offset.y );

		coords.push_back( 0.0f ); //s
		coords.push_back( 0.0f ); //t
		coords.push_back( offset.x ); //x
		coords.push_back( static_cast<float>(bitmap.rows) + offset.y ); //y

		coords.push_back( x );
		coords.push_back( 0.0f );
		coords.push_back( static_cast<float>(bitmap.width) + offset.x );
		coords.push_back( static_cast<float>(bitmap.rows) + offset.y );

		// Can't delete them while they are used, delete when font is cleaned
		font.AddGlyph( glyph );
		font.setCharWidth( ch, static_cast<float>( face->glyph->advance.x >> 6 ) );
	}

	glBindVertexArray(font.getVAO());
	glBindBuffer(GL_ARRAY_BUFFER, font.getVBO());
	glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(float), &coords[0], GL_STATIC_DRAW );

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		4*sizeof(float),    // stride
		reinterpret_cast<void*>(0) // array buffer offset
	);

	glVertexAttribPointer(
		1,                  // attribute 1
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		4*sizeof(float),    // stride
		reinterpret_cast<void*>(8) // array buffer offset
	);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return true;
}
