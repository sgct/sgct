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

#ifndef _TEXTURE_MANAGER_H_
#define _TEXTURE_MANAGER_H_

#include <vector>
#include <string>

namespace sgct //simple graphics cluster toolkit
{

class TextureManager
{
public:
	/*! Get the TextureManager instance */
	static TextureManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new TextureManager();
		}

		return mInstance;
	}

	/*! Destroy the TextureManager */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	const unsigned int getTextureByIndex(const unsigned int index);
	const unsigned int getTextureByName(const std::string name);

	void setAnisotropicFilterSize(float fval);
	void setAlphaModeForSingleChannelTextures(bool alpha) {mAlphaMode = alpha;}
	void setCompression(bool state);
	bool loadTexure(unsigned int &index, const std::string name, const std::string filename, bool interpolate, int mipmapLevels = 4);
	bool loadTexure(const std::string name, const std::string filename, bool interpolate, int mipmapLevels = 4);

private:
	TextureManager();
	~TextureManager();

	void freeTextureData();

	// Don't implement these, should give compile warning if used
	TextureManager( const TextureManager & tm );
	const TextureManager & operator=(const TextureManager & rhs );

private:
	bool getIndexByName(unsigned int &index, const std::string name);

	static TextureManager * mInstance;
	
	float mAnisotropicFilterSize;
	bool mCompression;
	bool mAlphaMode;
	std::vector< std::pair<std::string, unsigned int> > mTextures;
};

}

#endif