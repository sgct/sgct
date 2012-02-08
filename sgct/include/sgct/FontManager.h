#ifndef _FONT_MANAGER_H_
#define _FONT_MANAGER_H_

#include <string>
#include <set>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "sgct/Font.h"

namespace sgct
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
	const Freetype::Font * GetFont( const std::string & name, unsigned int height = mDefaultHeight );

	void SetDefaultFontPath( const std::string & path );

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
	std::set<Freetype::Font>::iterator CreateFont( const std::string & fontName, unsigned int height );
	bool MakeDisplayList( FT_Face face, char ch, Freetype::Font & font );

private:

	static FontManager * mInstance;					// Singleton instance of the LogManager
	static const FT_Short mDefaultHeight;			// Default height of font faces in pixels
	
	std::string mDefaultFontPath;			// The default font path from where to look for font files

	FT_Library  mFTLibrary;							// Freetype library

	std::map<std::string, std::string> mFontPaths;	// Holds all predefined font paths for generating font glyphs
	std::set<Freetype::Font> mFonts;				// All generated fonts
};

} // sgct

#endif