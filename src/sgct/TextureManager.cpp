/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <stdio.h>
#include <GL/glew.h>

#include "../include/sgct/TextureManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"

sgct::TextureManager * sgct::TextureManager::mInstance = NULL;

sgct_core::TextureData::TextureData()
{
	reset();
}

sgct_core::TextureData::~TextureData()
{
	reset();
}

void sgct_core::TextureData::reset()
{
	mId = GL_FALSE;
	mName.assign("NOTSET");
	mPath.assign("NOTSET");
	mDim[0] = -1;
	mDim[1] = -1;
	mDim[2] = -1;
}

sgct::TextureManager::TextureManager()
{
	setAnisotropicFilterSize(1.0f);
	setCompression(No_Compression);
	setAlphaModeForSingleChannelTextures(false);
	mWarpMode[0] = GL_CLAMP_TO_EDGE;
	mWarpMode[1] = GL_CLAMP_TO_EDGE;

	mOverWriteMode = true;

	//add empty texture
	sgct_core::TextureData tmpTexture;
	mTextures.push_back(tmpTexture);
}

sgct::TextureManager::~TextureManager()
{
	freeTextureData();
}

/*!
	This function is for direct access to a texture in TextureManager's storage.
	
	\param handle to texture
	\returns openGL texture id if handle exists otherwise GL_FALSE/0.
*/
const unsigned int sgct::TextureManager::getTextureByHandle(const std::size_t handle)
{
	return handle >= mTextures.size() ? 0 : mTextures[handle].mId;
}

/*!
	This function searches for a specific texute and returns a texture handle

	\param index/handle to texture
	\param name to search for
	\returns true if texture index/handle is found
*/
bool sgct::TextureManager::getIndexByName(std::size_t &handle, const std::string name)
{
	for(unsigned int i=0; i<mTextures.size(); i++)
		if( mTextures[i].mName.compare(name) == 0 )
		{
			handle = i;
			return true;
		}

	handle = 0;
	return false;
}

/*!
	This function performs a search for a texture by it's name. If many textures are stored in the
	TextureManager then it might be better to use direct access using \link getTextureByIndex \endlink

	\param name of texture
	\returns openGL texture id if texture is found otherwise GL_FALSE/0.
*/
const unsigned int sgct::TextureManager::getTextureByName(const std::string name)
{
	for(unsigned int i=0; i<mTextures.size(); i++)
	if (mTextures[i].mName.compare(name) == 0)
			return mTextures[i].mId;
	return 0;
}

/*!
Get the texture name. If not found then "NOT_FOUND" is returned.
*/
const std::string sgct::TextureManager::getTextureName(const std::size_t handle)
{
	if (handle < mTextures.size())
	{
		return mTextures[handle].mName;
	}
	else
		return std::string("NOT_FOUND");
}

/*!
Get the texture path. If not found then "NOT_FOUND" is returned.
*/
const std::string sgct::TextureManager::getTexturePath(const std::size_t handle)
{
	if (handle < mTextures.size())
	{
		return mTextures[handle].mPath;
	}
	else
		return std::string("NOT_FOUND");
}

/*!
Get the texture path. If not found then "NOT_FOUND" is returned.
*/
const std::string sgct::TextureManager::getTexturePath(const std::string name)
{
	for (unsigned int i = 0; i<mTextures.size(); i++)
		if (mTextures[i].mName.compare(name) == 0)
			return mTextures[i].mPath;
	return std::string("NOT_FOUND");
}

/*!
Get the dimensions of a texture by handle. If not found all variables will be set to -1.
*/
void sgct::TextureManager::getDimensions(const std::size_t handle, int & x, int & y, int & channels)
{
	if (handle < mTextures.size())
	{
		x = mTextures[handle].mDim[0];
		y = mTextures[handle].mDim[1];
		channels = mTextures[handle].mDim[2];
		return;
	}

	//if not found
	x = -1;
	y = -1;
	channels = -1;
}

/*!
Get the dimensions of a texture by name. If not found all variables will be set to -1.
*/
void sgct::TextureManager::getDimensions(const std::string name, int & x, int & y, int & channels)
{
	for (unsigned int i = 0; i<mTextures.size(); i++)
		if (mTextures[i].mName.compare(name) == 0)
		{
			x = mTextures[i].mDim[0];
			y = mTextures[i].mDim[1];
			channels = mTextures[i].mDim[2];
			return;
		}
		
	//if not found
	x = -1;
	y = -1;
	channels = -1;
}

/*!
	Sets the anisotropic filter size. Default is 1.0 (isotropic) which disables anisotropic filtering.
	This filtering mode can slow down performace. For more info look at: <a href="http://en.wikipedia.org/wiki/Anisotropic_filtering">Anisotropic filtering</a>
*/
void sgct::TextureManager::setAnisotropicFilterSize(float fval)
{
	//get max
	float maximumAnistropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnistropy);

	if( fval >= 1.0f && fval <= maximumAnistropy )
		mAnisotropicFilterSize = fval;
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING,
			"TextureManager warning: Anisotropic filtersize=%.2f is incorrect.\nMax and min values for your hardware is %.1f and 1.0.\n",
			maximumAnistropy);
	}
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

/*!
\returns the current compression mode
*/
sgct::TextureManager::CompressionMode sgct::TextureManager::getCompression()
{
	return mCompression;
}

/*!
	Load a texture to the TextureManager.
	\param name the name of the texture
	\param filename the filename or path to the texture
	\param interpolate set to true for using interpolation (bi-linear filtering)
	\param mipmapLevels is the number of mipmap levels that will be generated, setting this value to 1 or less disables mipmaps
	\return true if texture loaded successfully
*/
bool sgct::TextureManager::loadTexure(const std::string name, const std::string filename, bool interpolate, int mipmapLevels)
{
	std::size_t tmpId = 0;

	return loadTexure(tmpId, name, filename, interpolate, mipmapLevels);
}

/*!
	Load a texture to the TextureManager.
	\param handle the handle to the texture
	\param name the name of the texture
	\param filename the filename or path to the texture
	\param interpolate set to true for using interpolation (bi-linear filtering)
	\param mipmapLevels is the number of mipmap levels that will be generated, setting this value to 1 or less disables mipmaps
	\return true if texture loaded successfully
*/
bool sgct::TextureManager::loadTexure(std::size_t &handle, const std::string name, const std::string filename, bool interpolate, int mipmapLevels)
{
	GLuint texID = 0;
	bool reload = false;
	sgct_core::TextureData tmpTexture;
	sgct_core::Image img;

	//check if texture exits in manager
	if( getIndexByName(handle, name) ) //texture with that name exists already
	{	
		if( mOverWriteMode )
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "TextureManager: Reloading texture '%s'! [id=%d]\n", filename.c_str(), getTextureByHandle( handle ) );
			
			texID = getTextureByHandle(handle);
			if( texID != 0 )
				glDeleteTextures(1, &texID);
			texID = 0;
			reload = true;
		}
		else
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "TextureManager: '%s' exists already! [id=%d]\n", filename.c_str(), getTextureByHandle( handle ) );
			return true;
		}
	}

	//load image
	if ( !img.load(filename.c_str()) )
	{
		if (reload)
		{
			mTextures[handle].reset();
		}
		else
			handle = 0;

		return false;
	}
	
	if (img.getData() != NULL)
	{
		glGenTextures( 1, &texID );
		glBindTexture(GL_TEXTURE_2D, texID);

		int textureType = GL_BGR;

		//if OpenGL 1-2
		if( Engine::instance()->isOGLPipelineFixed() )
		{
			if (img.getChannels() == 4)	textureType = GL_BGRA;
			else if (img.getChannels() == 1)	textureType = (mAlphaMode ? GL_ALPHA : GL_LUMINANCE);
			else if (img.getChannels() == 2)	textureType = GL_LUMINANCE_ALPHA;
		}
		else //OpenGL 3+
		{
			if (img.getChannels() == 4)	textureType = GL_BGRA;
			else if (img.getChannels() == 1)	textureType = GL_RED;
			else if (img.getChannels() == 2)	textureType = GL_RG;
		}

		GLint internalFormat;

		switch (img.getChannels())
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
			if( Engine::instance()->isOGLPipelineFixed() )
			{
				if( mCompression == No_Compression)
					internalFormat = GL_LUMINANCE8_ALPHA8;
				else
					internalFormat = GL_COMPRESSED_LUMINANCE_ALPHA;
			}
			else
			{
				if( mCompression == No_Compression)
					internalFormat = GL_RG8;
				else if( mCompression == Generic)
					internalFormat = GL_COMPRESSED_RG;
				else
					internalFormat = GL_COMPRESSED_RG_RGTC2;
			}
			break;
		case 1:
			if( Engine::instance()->isOGLPipelineFixed() )
				internalFormat = (mCompression == No_Compression) ? (mAlphaMode ? GL_ALPHA8 : GL_LUMINANCE8) : (mAlphaMode ? GL_COMPRESSED_ALPHA : GL_COMPRESSED_LUMINANCE);
			else
			{
				if( mCompression == No_Compression)
					internalFormat = GL_R8;
				else if( mCompression == Generic)
					internalFormat = GL_COMPRESSED_RED;
				else
					internalFormat = GL_COMPRESSED_RED_RGTC1;
			}
			break;

		default:
			internalFormat = GL_RGBA8;
			break;
		}
		
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "TextureManager: Creating texture... size: %dx%d, %d-channels compression: %s\n",
			img.getSizeX(),
			img.getSizeY(),
			img.getChannels(),
			(mCompression == No_Compression) ? "none" : ((mCompression == Generic) ? "generic" : "S3TC/DXT"));

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        if(mipmapLevels <= 1)
            mipmapLevels = 1;
        
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.getSizeX(), img.getSizeY(), 0, textureType, GL_UNSIGNED_BYTE, img.getData());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels-1);
		
		if(mipmapLevels > 1)
		{
			glGenerateMipmap( GL_TEXTURE_2D ); //allocate the mipmaps
            
            GLfloat maxAni;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni);
			//sgct::MessageHandler::instance()->print("Max anisotropy: %f\n", maxAni);
			
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, mAnisotropicFilterSize > maxAni ? maxAni : mAnisotropicFilterSize);
		}
		else
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
		}

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWarpMode[0] );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWarpMode[1] );

		tmpTexture.mId = texID;
		tmpTexture.mName.assign(name);
		tmpTexture.mPath.assign(filename);
		tmpTexture.mDim[0] = img.getSizeX();
		tmpTexture.mDim[1] = img.getSizeY();
		tmpTexture.mDim[2] = img.getChannels();

		if(!reload)
		{
			mTextures.push_back(tmpTexture);
			handle = mTextures.size()-1;
		}
		else if (handle != 0) //valid handle
		{
			mTextures[handle] = tmpTexture;
		}

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "TextureManager: Texture created from '%s' [id=%d]\n", filename.c_str(), texID );
		img.cleanup();
	}
	else //image data not valid
	{
		handle = 0;
		return false;
	}

	return true;
}

/*!
Load a unmanged texture. Note that this type of textures doesn't auto destruct.
\param texID the openGL texture id
\param filename the filename or path to the texture
\param interpolate set to true for using interpolation (bi-linear filtering)
\param mipmapLevels is the number of mipmap levels that will be generated, setting this value to 1 or less disables mipmaps
\return true if texture loaded successfully
*/
bool sgct::TextureManager::loadUnManagedTexture(unsigned int & texID, const std::string filename, bool interpolate, int mipmapLevels)
{
	unsigned int tmpTexID = GL_FALSE;
	
	//load image
	sgct_core::Image img;
	if (!img.load(filename.c_str()))
	{
		return false;
	}

	if (img.getData() != NULL)
	{
		glGenTextures(1, &tmpTexID);
		glBindTexture(GL_TEXTURE_2D, tmpTexID);

		int textureType = GL_BGR;

		//if OpenGL 1-2
		if (Engine::instance()->isOGLPipelineFixed())
		{
			if (img.getChannels() == 4)	textureType = GL_BGRA;
			else if (img.getChannels() == 1)	textureType = (mAlphaMode ? GL_ALPHA : GL_LUMINANCE);
			else if (img.getChannels() == 2)	textureType = GL_LUMINANCE_ALPHA;
		}
		else //OpenGL 3+
		{
			if (img.getChannels() == 4)	textureType = GL_BGRA;
			else if (img.getChannels() == 1)	textureType = GL_RED;
			else if (img.getChannels() == 2)	textureType = GL_RG;
		}

		GLint internalFormat;

		switch (img.getChannels())
		{
		case 4:
		{
				  if (mCompression == No_Compression)
					  internalFormat = GL_RGBA8;
				  else if (mCompression == Generic)
					  internalFormat = GL_COMPRESSED_RGBA;
				  else
					  internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		}
			break;
		case 3:
		{
				  if (mCompression == No_Compression)
					  internalFormat = GL_RGB8;
				  else if (mCompression == Generic)
					  internalFormat = GL_COMPRESSED_RGB;
				  else
					  internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
			break;
		case 2:
			if (Engine::instance()->isOGLPipelineFixed())
			{
				if (mCompression == No_Compression)
					internalFormat = GL_LUMINANCE8_ALPHA8;
				else
					internalFormat = GL_COMPRESSED_LUMINANCE_ALPHA;
			}
			else
			{
				if (mCompression == No_Compression)
					internalFormat = GL_RG8;
				else if (mCompression == Generic)
					internalFormat = GL_COMPRESSED_RG;
				else
					internalFormat = GL_COMPRESSED_RG_RGTC2;
			}
			break;
		case 1:
			if (Engine::instance()->isOGLPipelineFixed())
				internalFormat = (mCompression == No_Compression) ? (mAlphaMode ? GL_ALPHA8 : GL_LUMINANCE8) : (mAlphaMode ? GL_COMPRESSED_ALPHA : GL_COMPRESSED_LUMINANCE);
			else
			{
				if (mCompression == No_Compression)
					internalFormat = GL_RED;
				else if (mCompression == Generic)
					internalFormat = GL_COMPRESSED_RED;
				else
					internalFormat = GL_COMPRESSED_RED_RGTC1;
			}
			break;

		default:
			internalFormat = GL_RGBA8;
			break;
		}

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "TextureManager: Creating texture... size: %dx%d, %d-channels compression: %s\n",
			img.getSizeX(),
			img.getSizeY(),
			img.getChannels(),
			(mCompression == No_Compression) ? "none" : ((mCompression == Generic) ? "generic" : "S3TC/DXT"));

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
		if(mipmapLevels <= 1)
            mipmapLevels = 1;
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.getSizeX(), img.getSizeY(), 0, textureType, GL_UNSIGNED_BYTE, img.getData());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels-1);

		if (mipmapLevels > 1)
		{
			glGenerateMipmap(GL_TEXTURE_2D); //allocate the mipmaps
            
            GLfloat maxAni;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni);
			//sgct::MessageHandler::instance()->print("Max anisotropy: %f\n", maxAni);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, mAnisotropicFilterSize > maxAni ? maxAni : mAnisotropicFilterSize);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWarpMode[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWarpMode[1]);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "TextureManager: Unmanaged texture created from '%s' [id=%d]\n", filename.c_str(), tmpTexID);
		img.cleanup();
	}
	else //image data not valid
	{
		return false;
	}

	texID = tmpTexID;
	return true;
}

void sgct::TextureManager::freeTextureData()
{
	//the textures might not be stored in a sequence so
	//let's erase them one by one
	for(unsigned int i=0; i<mTextures.size(); i++)
		if(mTextures[i].mId) //if set, delete
			glDeleteTextures(1, &mTextures[i].mId);
	mTextures.clear();
}
