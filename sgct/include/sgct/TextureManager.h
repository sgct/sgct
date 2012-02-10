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
	void setCompression(bool state);
	bool loadTexure(unsigned int &index, const std::string name, const std::string filename, bool interpolate, int mipmapLevels = 4);

private:
	TextureManager();
	~TextureManager();

	void freeTextureData();

	// Don't implement these, should give compile warning if used
	TextureManager( const TextureManager & tm );
	const TextureManager & operator=(const TextureManager & rhs );

private:

	static TextureManager * mInstance;
	
	float mAnisotropicFilterSize;
	bool mCompression;
	std::vector< std::pair<std::string, unsigned int> > mTextures;
};

}

#endif