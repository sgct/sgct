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

/*! \namespace sgct_text
\brief simple graphics cluster toolkit text namespace.
This namespace is used for text rendering and font management.
*/
namespace sgct_text
{

/*!
Singleton for font handling. A lot of the font handling is based on Nehes tutorials for freetype
@link http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/
*/
class FontManager
{
public:
	// Convinience enum from where to load font files
	enum FontPath{ FontPath_Local, FontPath_Default };

	~FontManager(void);

	bool AddFont( const std::string & fontName, std::string path, FontPath fontPath = FontPath_Default );
	const Font * GetFont( const std::string & name, unsigned int height = mDefaultHeight );
	const Font * GetDefaultFont( unsigned int height = mDefaultHeight );

	void SetDefaultFontPath( const std::string & path );
	sgct::ShaderProgram getShader() { return mShader; }
	unsigned int getMVPLoc() { return mMVPLoc; }
	unsigned int getColLoc() { return mColLoc; }
	unsigned int getTexLoc() { return mTexLoc; }

	static FontManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new FontManager();
		}

		return mInstance;
	}

	/*! Destroy the FontManager */
	static void Destroy()
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
	std::set<Font>::iterator CreateFont( const std::string & fontName, unsigned int height );
	bool MakeDisplayList( FT_Face face, char ch, Font & font );
	bool MakeVBO( FT_Face face, Font & font );

	// Don't implement these, should give compile warning if used
	FontManager( const FontManager & fm );
	const FontManager & operator=(const FontManager & rhs );

private:

	static FontManager * mInstance;			// Singleton instance of the LogManager
	static const FT_Short mDefaultHeight;	// Default height of font faces in pixels

	std::string mDefaultFontPath;			// The default font path from where to look for font files

	FT_Library  mFTLibrary;					// Freetype library

	std::map<std::string, std::string> mFontPaths;	// Holds all predefined font paths for generating font glyphs
	std::set<Font> mFonts;				// All generated fonts

	sgct::ShaderProgram mShader;
	int mMVPLoc, mColLoc, mTexLoc;
};

} // sgct

#endif
