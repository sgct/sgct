/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _FONT_MANAGER_H_
#define _FONT_MANAGER_H_

#include <string>
#include <set>
#include <map>

#include "Font.h"
#include "ShaderProgram.h"
#include <glm/glm.hpp>

/*! \namespace sgct_text
\brief SGCT text namespace is used for text rendering and font management
*/
namespace sgct_text
{

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
if( !sgct_text::FontManager::instance()->addFont( "Special", "Special.ttf", "myfonts/" ) )
	sgct_text::FontManager::instance()->getFont( "Special", 14 );
\endcode
\n
Then in the draw or draw2d callback the font can be rendered:
\code{.cpp}
sgct_text::print(sgct_text::FontManager::instance()->getFont( "Verdana", 14 ), 50, 50, "Hello World!");
\endcode
\n
SGCT has an internal font that can be used as well:
\code{.cpp}
sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", 14 ), 50, 50, "Hello World!");
\endcode
*/
class FontManager
{
public:
	// Convinience enum from where to load font files
	enum FontPath{ FontPath_Local, FontPath_Default };

	~FontManager(void);

	bool addFont( const std::string & fontName, std::string path, FontPath fontPath = FontPath_Default );
	const Font * getFont( const std::string & name, unsigned int height = mDefaultHeight );
	const Font * getDefaultFont( unsigned int height = mDefaultHeight );

	void setDefaultFontPath( const std::string & path );
	void setStrokeSize( signed long size );
	void setStrokeColor( glm::vec4 color );
	void setDrawInScreenSpace( bool state );

	inline glm::vec4 getStrokeColor() { return mStrokeColor; }
	inline signed long getStrokeSize() { return mStrokeSize; }
	inline bool getDrawInScreenSpace() { return mDrawInScreenSpace; }

	sgct::ShaderProgram getShader() { return mShader; }
	inline unsigned int getMVPLoc() { return mMVPLoc; }
	inline unsigned int getColLoc() { return mColLoc; }
	inline unsigned int getStkLoc() { return mStkLoc; }
	inline unsigned int getTexLoc() { return mTexLoc; }

	static FontManager * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new FontManager();
		}

		return mInstance;
	}

	/*! Destroy the FontManager */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

private:
	FontManager(void);

	/// Helper functions
	std::set<Font>::iterator createFont( const std::string & fontName, unsigned int height );
	bool makeDisplayList( FT_Face face, char ch, Font & font );
	bool makeVBO( FT_Face face, Font & font );

	// Don't implement these, should give compile warning if used
	FontManager( const FontManager & fm );
	const FontManager & operator=(const FontManager & rhs );

private:

	static FontManager * mInstance;			// Singleton instance of the LogManager
	static const FT_Short mDefaultHeight;	// Default height of font faces in pixels

	std::string mDefaultFontPath;			// The default font path from where to look for font files

	FT_Library  mFTLibrary;					// Freetype library
	FT_Fixed mStrokeSize;
	glm::vec4 mStrokeColor;

	bool mDrawInScreenSpace;

	std::map<std::string, std::string> mFontPaths;	// Holds all predefined font paths for generating font glyphs
	std::set<Font> mFonts;				// All generated fonts

	sgct::ShaderProgram mShader;
	int mMVPLoc, mColLoc, mStkLoc, mTexLoc;
};

} // sgct

#endif
