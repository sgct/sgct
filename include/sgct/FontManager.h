/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.
 
Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#ifndef _FONT_MANAGER_H_
#define _FONT_MANAGER_H_

#include <string>
#include <set>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Font.h"

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

	// Don't implement these, should give compile warning if used
	FontManager( const FontManager & fm );
	const FontManager & operator=(const FontManager & rhs );

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
