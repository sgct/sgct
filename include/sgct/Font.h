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

#ifndef _FREETYPE_FONT_H_
#define _FREETYPE_FONT_H_

#include <freetype/ftglyph.h>

#include <vector>
#include <string>

namespace Freetype
{

/*!
Will ahandle font textures and rendering. Implementation is based on
Nehes font tutorial for freetype.
@link http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/
*/
class Font
{
public:
	Font( const std::string & fontName = std::string(), float height = 0.0f );
	~Font();

	void init( const std::string & fontName, unsigned int h );
	void clean();

	/*! Get the list base index */
	inline unsigned int getListBase() const { return mListBase; }

	/*! Get height of the font */
	inline float getHeight() const { return mHeight; }

	/*! Get the texture id's */
	inline const unsigned int * getTextures() const { return mTextures; }

	/*! Adds a glyph to the font */
	inline void AddGlyph( const FT_Glyph & glyph ){ mGlyphs.push_back( glyph ); }

public:

	/*! Less then Font comparison operator */
	inline bool operator<( const Font & rhs ) const
	{ return mName.compare( rhs.mName ) < 0 || (mName.compare( rhs.mName ) == 0 && mHeight < rhs.mHeight); }

	/*! Equal to Font comparison operator */
	inline bool operator==( const Font & rhs ) const
	{ return mName.compare( rhs.mName ) == 0 && mHeight == rhs.mHeight; }

private:
	std::string mName;				// Holds the font name
	float mHeight;					// Holds the height of the font.
	unsigned int * mTextures;		// Holds the texture id's
	unsigned int mListBase;			// Holds the first display list id

	std::vector<FT_Glyph> mGlyphs;	// All glyphs needed by the font
};

} // sgct

#endif
