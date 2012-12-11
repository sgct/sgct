/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
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
	setCompression(No_Compression);
	setAlphaModeForSingleChannelTextures(false);
	mWarpMode[0] = GL_CLAMP_TO_EDGE;
	mWarpMode[1] = GL_CLAMP_TO_EDGE;
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

/*!
	Set texture compression. Can be one of the following:
	- sgct::TextureManager::No_Compression
	- sgct::TextureManager::Generic
	- sgct::TextureManager::S3TC_DXT

	@param cm the compression mode
*/
void sgct::TextureManager::setCompression(CompressionMode cm)
{
	mCompression = cm;
}

/*!
	Set the OpenGL texture warping mode. Can be one of the following:
	- GL_CLAMP_TO_EDGE (Default)
	- GL_CLAMP_TO_BORDER
	- GL_MIRRORED_REPEAT
	- GL_REPEAT

	@param warp_s warping parameter along the s-axis (x-axis) 
	@param warp_t warping parameter along the t-axis (y-axis)
*/
void sgct::TextureManager::setWarpingMode(int warp_s, int warp_t)
{
	mWarpMode[0] = warp_s;
	mWarpMode[1] = warp_t;
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
	sgct_core::Image img;
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

		GLint internalFormat;

		switch(img.getChannels())
		{
		case 4:
			{
				if( mCompression == No_Compression)
					internalFormat = GL_RGBA8;
				else if( mCompression == Generic)
					internalFormat = GL_COMPRESSED_RGBA;
				else
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			}
			break;
		case 3:
			{
				if( mCompression == No_Compression)
					internalFormat = GL_RGB8;
				else if( mCompression == Generic)
					internalFormat = GL_COMPRESSED_RGB;
				else
					internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
			break;
		case 2:
			internalFormat = (mCompression == No_Compression) ? GL_LUMINANCE8_ALPHA8 : GL_COMPRESSED_LUMINANCE_ALPHA;
			break;
		case 1:
			internalFormat = (mCompression == No_Compression) ? (mAlphaMode ? GL_ALPHA8 : GL_LUMINANCE8) : (mAlphaMode ? GL_COMPRESSED_ALPHA : GL_COMPRESSED_LUMINANCE);
			break;

		default:
			internalFormat = GL_RGBA8;
			break;
		}
		
		sgct::MessageHandler::Instance()->print("Creating texture... size: %dx%d, %d-channels compression: %s\n",
			img.getSizeX(),
			img.getSizeY(),
			img.getChannels(),
			(mCompression == No_Compression) ? "none" : ((mCompression == Generic) ? "generic" : "S3TC/DXT"));

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.getSizeX(), img.getSizeY(), 0, textureType, GL_UNSIGNED_BYTE, img.getData());
		
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

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWarpMode[0] );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWarpMode[1] );

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
