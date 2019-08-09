/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__TEXTURE_MANAGER__H__
#define __SGCT__TEXTURE_MANAGER__H__

#include <sgct/Image.h>

#include <string>
#include <unordered_map>

namespace sgct_core {

struct TextureData {
    void reset();

    std::string mPath = "NOTSET";
    unsigned int mId = 0;
    int mWidth = -1;
    int mHeight = -1;
    int mChannels = -1;
};

} // namespace sgct_core

namespace sgct {

/*!
    The TextureManager loads and handles textures. It is a singleton and can be accessed
    anywhere using its static instance. Currently only PNG textures are supported.
*/
class TextureManager {
public:
    /*!
        The compression mode modes. For more info about texute compression look here:
        <a href="http://en.wikipedia.org/wiki/S3_Texture_Compression">S3 Texture compression</a>
    */
    enum CompressionMode {
        No_Compression = 0,
        Generic,
        S3TC_DXT
    };

    /*! Get the TextureManager instance */
    static TextureManager * instance();

    /*! Destroy the TextureManager */
    static void destroy();

    unsigned int getTextureId(const std::string& name);
    std::string getTexturePath(const std::string& name);
    void getDimensions(const std::string& name, int& width, int& height, int& channels);

    void setAlphaModeForSingleChannelTextures(bool alpha);
    void setOverWriteMode(bool mode);
    void setAnisotropicFilterSize(float fval);
    void setCompression(CompressionMode cm);
    void setWarpingMode(int warp_s, int warp_t);
    CompressionMode getCompression();
    bool loadTexture(const std::string& name, const std::string& filename,
        bool interpolate, int mipmapLevels = 8);
    bool loadTexture(const std::string& name, sgct_core::Image* imgPtr, bool interpolate,
        int mipmapLevels = 8);
    bool loadUnManagedTexture(unsigned int& texID, const std::string& filename,
        bool interpolate, int mipmapLevels = 8);

private:
    TextureManager();
    ~TextureManager();
    bool updateTexture(const std::string& name, unsigned int* texPtr, bool* reload);
    bool uploadImage(sgct_core::Image* imgPtr, unsigned int* texPtr);
    void freeTextureData();

    static TextureManager* mInstance;
    
    float mAnisotropicFilterSize;
    CompressionMode mCompression = No_Compression;
    bool mAlphaMode = false;
    bool mOverWriteMode = true;
    bool mInterpolate = true;
    std::unordered_map<std::string, sgct_core::TextureData> mTextures;
    int mMipmapLevels = 8;
    int mWarpMode[2];
};

} // namespace sgct_core

#endif // __SGCT__TEXTURE_MANAGER__H__
