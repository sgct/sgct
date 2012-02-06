#ifndef _TEXTURE_MANAGER_H_
#define _TEXTURE_MANAGER_H_

#include <vector>
#include <string>

namespace sgct //small graphics cluster toolkit
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
			mInstance->setAnisotropicFilterSize(1.0f);
			mInstance->setCompression(false);
		}

		return mInstance;
	}

	/*! Destroy the TextureManager */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			mInstance->freeTextureData();
			delete mInstance;
			mInstance = NULL;
		}
	}

	const unsigned int getTextureByIndex(const unsigned int index);
	const unsigned int getTextureByName(const std::string name);

	void setAnisotropicFilterSize(float fval);
	void setCompression(bool state);
	bool loadTexure(unsigned int &index, const std::string name, const std::string filename, bool interpolate, int mipmapLevels = 4);

private:
	void freeTextureData();

	static TextureManager * mInstance;
	
	float mAnisotropicFilterSize;
	bool mCompression;
	std::vector< std::pair<std::string, unsigned int> > mTextures;
};

}

#endif