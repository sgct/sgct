/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _TEXTURE_MANAGER_H_
#define _TEXTURE_MANAGER_H_

#include <string>

#include "Image.h"
#include "helpers/SGCTCPPEleven.h"

namespace sgct_core
{
class TextureData
{
public:
	TextureData();
	~TextureData();
	void reset();

	std::string mPath;
	unsigned int mId;
	int mWidth;
	int mHeight;
	int mChannels;
};
}

namespace sgct //simple graphics cluster toolkit
{

/*!
	The TextureManager loads and handles textures. It is a singleton and can be accessed anywhere using its static instance. Currently only PNG textures are supported.
*/
class TextureManager
{
public:
	/*!
		The compression mode modes. For more info about texute compression look here: <a href="http://en.wikipedia.org/wiki/S3_Texture_Compression">S3 Texture compression</a>
	*/
	enum CompressionMode { No_Compression = 0, Generic, S3TC_DXT };

	/*! Get the TextureManager instance */
	static TextureManager * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new TextureManager();
		}

		return mInstance;
	}

	/*! Destroy the TextureManager */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	const unsigned int getTextureId(const std::string name);
	const std::string getTexturePath(const std::string name);
	void getDimensions(const std::string name, int & width, int & height, int & channels);

	/*!
		Sets if a single channel texture should be interpreted as alpha or luminance.
	*/
	void setAlphaModeForSingleChannelTextures(bool alpha) {mAlphaMode = alpha;}

	/*!
		Sets if loading a texture with an existing name should be overwritten or not.
	*/
	void setOverWriteMode(bool mode) {mOverWriteMode = mode;}

	void setAnisotropicFilterSize(float fval);
	void setCompression(CompressionMode cm);
	void setWarpingMode(int warp_s, int warp_t);
	CompressionMode getCompression();
	bool loadTexure(const std::string name, const std::string filename, bool interpolate, int mipmapLevels = 8);
	bool loadTexure(const std::string name, sgct_core::Image * imgPtr, bool interpolate, int mipmapLevels = 8);
	bool loadUnManagedTexture(unsigned int & texID, const std::string filename, bool interpolate, int mipmapLevels = 8);

private:
	TextureManager();
	~TextureManager();
	bool updateTexture(const std::string & name, unsigned int * texPtr, bool * reload);
	bool uploadImage(sgct_core::Image * imgPtr, unsigned int * texPtr);

	void freeTextureData();

	// Don't implement these, should give compile warning if used
	TextureManager( const TextureManager & tm );
	const TextureManager & operator=(const TextureManager & rhs );

private:
	static TextureManager * mInstance;
	
	float mAnisotropicFilterSize;
	CompressionMode mCompression;
	bool mAlphaMode;
	bool mOverWriteMode;
	bool mInterpolate;
	sgct_cppxeleven::unordered_map<std::string, sgct_core::TextureData> mTextures;
	int mMipmapLevels;
	int mWarpMode[2];
};

}

#endif
