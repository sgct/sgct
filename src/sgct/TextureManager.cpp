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

#include <stdio.h>
#include <GL/glew.h>

#include "../include/sgct/TextureManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Image.h"

sgct::TextureManager * sgct::TextureManager::mInstance = NULL;

sgct::TextureManager::TextureManager()
{
	setAnisotropicFilterSize(1.0f);
	setCompression(false);
	setAlphaModeForSingleChannelTextures(false);
}

sgct::TextureManager::~TextureManager()
{
	freeTextureData();
}

const unsigned int sgct::TextureManager::getTextureByIndex(const unsigned int index)
{
	return index >= mTextures.size() ? 0 : mTextures[index].second;
}

bool sgct::TextureManager::getIndexByName(unsigned int &index, const std::string name)
{
	for(unsigned int i=0; i<mTextures.size(); i++)
		if( mTextures[i].first.compare(name) == 0 )
		{
			index = i;
			return true;
		}

	index = 0;
	return false;
}

const unsigned int sgct::TextureManager::getTextureByName(const std::string name)
{
	for(unsigned int i=0; i<mTextures.size(); i++)
		if( mTextures[i].first.compare(name) == 0 )
			return mTextures[i].second;
	return 0;
}

void sgct::TextureManager::setAnisotropicFilterSize(float fval)
{
	//get max
	float maximumAnistropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnistropy);

	if( fval >= 1.0f && fval <= maximumAnistropy )
		mAnisotropicFilterSize = fval;

}

void sgct::TextureManager::setCompression(bool state)
{
	mCompression = state;
}

bool sgct::TextureManager::loadTexure(const std::string name, const std::string filename, bool interpolate, int mipmapLevels)
{
	unsigned int tmpId = 0;

	return loadTexure(tmpId, name, filename, interpolate, mipmapLevels);
}

bool sgct::TextureManager::loadTexure(unsigned int &index, const std::string name, const std::string filename, bool interpolate, int mipmapLevels)
{
	GLuint texID = 0;

	//check if texture exits in manager
	if( getIndexByName(index, name) ) //texture with that name exists already
	{
		sgct::MessageHandler::Instance()->print("Texture '%s' exists already! [id=%d]\n", filename.c_str(), getTextureByIndex( index ) );
		return true;
	}

	//load image
	core_sgct::Image img;
	if( !img.load(filename.c_str()) )
		return false;

	glGenTextures( 1, &texID );

	if(img.getData() != NULL)
	{
		glBindTexture(GL_TEXTURE_2D, texID);

		int textureType = GL_RGB;
		if(img.getChannels() == 4)	textureType = GL_RGBA;
		else if(img.getChannels() == 1)	textureType = (mAlphaMode ? GL_ALPHA : GL_LUMINANCE);
		else if(img.getChannels() == 2)	textureType = GL_LUMINANCE_ALPHA;

		GLint components;

		if(mCompression)
		{
			if( img.getChannels() == 4 )
			{
				components = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				sgct::MessageHandler::Instance()->print("Creating texture... size: %dx%d, %d-channels DXT3 compression\n", img.getSizeX(), img.getSizeY(), img.getChannels() );
			}
			else if( img.getChannels() == 3 )
			{
				components = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				sgct::MessageHandler::Instance()->print("Creating texture... size: %dx%d, %d-channels DXT1 compression\n", img.getSizeX(), img.getSizeY(), img.getChannels() );
			}
			else
			{
				components = img.getChannels();
				sgct::MessageHandler::Instance()->print("Creating texture... size: %dx%d, %d-channels no compression\n", img.getSizeX(), img.getSizeY(), img.getChannels() );
			}
		}
		else
		{
			components = img.getChannels();
			sgct::MessageHandler::Instance()->print("Creating texture... size: %dx%d, %d-channels no compression\n", img.getSizeX(), img.getSizeY(), img.getChannels() );
		}

		glTexImage2D(GL_TEXTURE_2D, 0, components, img.getSizeX(), img.getSizeY(), 0, textureType, GL_UNSIGNED_BYTE, img.getData());
		
		if(mipmapLevels > 1)
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
			//openGL ver >= 3.0
			glGenerateMipmap( GL_TEXTURE_2D );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, mAnisotropicFilterSize);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels);
		}
		else
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
		}

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		mTextures.push_back( std::pair<std::string, unsigned int>( name, (unsigned int)texID ) );

		sgct::MessageHandler::Instance()->print("Texture created from '%s' [id=%d]\n", filename.c_str(), texID );
		img.cleanup();
	}
	else //image data not valid
		return false;

    index = mTextures.size()-1;
	return true;
}

void sgct::TextureManager::freeTextureData()
{
	//the textures might not be stored in a sequence so
	//let's erase them one by one
	for(unsigned int i=0; i<mTextures.size(); i++)
		glDeleteTextures(1, &mTextures[i].second);
	mTextures.clear();
}
