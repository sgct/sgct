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

#include <GL/glew.h>
#if __WIN32__
#include <GL/wglew.h>
#elif __LINUX__
#include <GL/glext.h>
#else
#include <OpenGL/glext.h>
#endif
#include <GL/glfw.h>
#include "../include/sgct/Font.h"

#define SAFE_DELETE( p ) { if( p != NULL ) { delete p; p = NULL; } };
#define SAFE_DELETE_A( p ) { if( p != NULL ) { delete[] p; p = NULL; } };

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
