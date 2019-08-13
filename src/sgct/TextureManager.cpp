/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/TextureManager.h>

#include <sgct/Engine.h>
#include <sgct/Image.h>
#include <sgct/MessageHandler.h>

namespace sgct {

void TextureManager::TextureData::reset() {
    *this = TextureData();
}

TextureManager* TextureManager::mInstance = nullptr;

TextureManager* TextureManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new TextureManager();
    }

    return mInstance;
}

/*! Destroy the TextureManager */
void TextureManager::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

TextureManager::TextureManager() {
    setAnisotropicFilterSize(1.f);
    mWarpMode[0] = GL_CLAMP_TO_EDGE;
    mWarpMode[1] = GL_CLAMP_TO_EDGE;

    TextureData tmpTexture;
    mTextures["NOTSET"] = tmpTexture;
}

TextureManager::~TextureManager() {
    freeTextureData();
}

/*!
    This function gets a texture id by it's name.

    \param name of texture
    \returns openGL texture id if texture is found otherwise GL_FALSE/0.
*/
unsigned int TextureManager::getTextureId(const std::string& name) {
    return mTextures.count(name) > 0 ? mTextures[name].mId : 0;
}

/*!
Get the texture path. If not found then "NOT_FOUND" is returned.
*/
std::string TextureManager::getTexturePath(const std::string& name) {
    return mTextures.count(name) > 0 ? mTextures[name].mPath : std::string("NOT_FOUND");
}

/*!
Get the dimensions of a texture by name. If not found all variables will be set to -1.
*/
void TextureManager::getDimensions(const std::string& name, int& width, int& height,
                                   int& channels) const
{
    if (mTextures.count(name) > 0) {
        const TextureData& texData = mTextures.at(name);
        width = texData.mWidth;
        height = texData.mWidth;
        channels = texData.mWidth;
    }
    else {
        width = -1;
        height = -1;
        channels = -1;
    }
}

/*!
    Sets if a single channel texture should be interpreted as alpha or luminance.
*/
void TextureManager::setAlphaModeForSingleChannelTextures(bool alpha) {
    mAlphaMode = alpha;
}

/*!
    Sets if loading a texture with an existing name should be overwritten or not.
*/
void TextureManager::setOverWriteMode(bool mode) {
    mOverWriteMode = mode;
}

/*!
    Sets the anisotropic filter size. Default is 1.0 (isotropic) which disables anisotropic filtering.
    This filtering mode can slow down performace. For more info look at: <a href="http://en.wikipedia.org/wiki/Anisotropic_filtering">Anisotropic filtering</a>
*/
void TextureManager::setAnisotropicFilterSize(float fval) {
    //get max
    float maximumAnistropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnistropy);

    if (fval >= 1.0f && fval <= maximumAnistropy) {
        mAnisotropicFilterSize = fval;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "TextureManager warning: Anisotropic filtersize=%.2f is incorrect.\nMax and "
            "min values for your hardware is %.1f and 1.0.\n",
            maximumAnistropy
        );
    }
}

/*!
    Set texture compression. Can be one of the following:
    - sgct::TextureManager::None
    - sgct::TextureManager::Generic
    - sgct::TextureManager::S3TC_DXT

    @param cm the compression mode
*/
void TextureManager::setCompression(CompressionMode cm) {
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
void TextureManager::setWarpingMode(int warp_s, int warp_t) {
    mWarpMode[0] = warp_s;
    mWarpMode[1] = warp_t;
}

/*!
\returns the current compression mode
*/
TextureManager::CompressionMode TextureManager::getCompression() const {
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
bool TextureManager::loadTexture(const std::string& name, const std::string& filename,
                                 bool interpolate, int mipmapLevels)
{
    GLuint texID = 0;
    bool reload = false;
    TextureData tmpTexture;
    sgct_core::Image img;

    mInterpolate = interpolate;
    mMipmapLevels = mipmapLevels;

    if (!updateTexture(name, &texID, &reload)) {
        return true;
    }
    
    std::unordered_map<std::string, TextureData>::iterator textureItem = mTextures.end();

    //load image
    if (!img.load(filename)) {
        if (reload) {
            textureItem->second.reset();
        }

        return false;
    }
    
    if (img.getData() != nullptr) {
        if (!uploadImage(&img, &texID)) {
            return false;
        }

        tmpTexture.mId = texID;
        tmpTexture.mPath.assign(filename);
        tmpTexture.mWidth = static_cast<int>(img.getWidth());
        tmpTexture.mHeight = static_cast<int>(img.getHeight());
        tmpTexture.mChannels = static_cast<int>(img.getChannels());

        if (!reload) {
            mTextures[name] = tmpTexture;
        }

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "TextureManager: Texture created from '%s' [id=%d]\n",
            filename.c_str(), texID
        );
    }
    else {
        // image data not valid
        return false;
    }

    return true;
}

/*!
Load a texture to the TextureManager.
\param name the name of the texture
\param imgPtr pointer to image object
\param interpolate set to true for using interpolation (bi-linear filtering)
\param mipmapLevels is the number of mipmap levels that will be generated, setting this value to 1 or less disables mipmaps
\return true if texture loaded successfully
*/
bool TextureManager::loadTexture(const std::string& name, sgct_core::Image* imgPtr,
                                 bool interpolate, int mipmapLevels)
{
    if (!imgPtr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "TextureManager: Cannot create texture '%s' from invalid image!\n",
            name.c_str()
        );
        return false;
    }
    
    GLuint texID = 0;
    bool reload = false;
    TextureData tmpTexture;

    mInterpolate = interpolate;
    mMipmapLevels = mipmapLevels;

    if (!updateTexture(name, &texID, &reload)) {
        return true;
    }

    if (imgPtr->getData() != nullptr) {
        if (!uploadImage(imgPtr, &texID)) {
            return false;
        }

        tmpTexture.mId = texID;
        tmpTexture.mPath.assign("NOTSET");
        tmpTexture.mWidth = static_cast<int>(imgPtr->getWidth());
        tmpTexture.mHeight = static_cast<int>(imgPtr->getHeight());
        tmpTexture.mChannels = static_cast<int>(imgPtr->getChannels());

        if (!reload) {
            mTextures[name] = tmpTexture;
        }

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "TextureManager: Texture created from image [id=%d]\n", texID
        );
    }
    else {
        //image data not valid
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
bool TextureManager::loadUnManagedTexture(unsigned int& texID,
                                          const std::string& filename, bool interpolate,
                                          int mipmapLevels)
{
    unsigned int tmpTexID = 0;
    mInterpolate = interpolate;
    mMipmapLevels = mipmapLevels;

    if (texID != 0) {
        glDeleteTextures(1, &texID);
        texID = 0;
    }
    
    //load image
    sgct_core::Image img;
    if (!img.load(filename)) {
        return false;
    }

    if (img.getData() != nullptr) {
        if (!uploadImage(&img, &tmpTexID)) {
            return false;
        }

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "TextureManager: Unmanaged texture created from '%s' [id=%d]\n",
            filename.c_str(), tmpTexID
        );
    }
    else {
        // image data not valid
        return false;
    }

    texID = tmpTexID;
    return true;
}

/*!
returns true if texture will be uploaded
*/
bool TextureManager::updateTexture(const std::string& name, unsigned int* texPtr,
                                   bool* reload)
{
    //check if texture exits in manager
    bool exist = mTextures.count(name) > 0;
    std::unordered_map<std::string, TextureData>::iterator textureItem = mTextures.end();

    if (exist) {
        //get it
        textureItem = mTextures.find(name);
        (*texPtr) = textureItem->second.mId;

        if (mOverWriteMode) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "TextureManager: Reloading texture '%s'! [id=%d]\n",
                name.c_str(), (*texPtr)
            );

            if ((*texPtr) != 0) {
                glDeleteTextures(1, texPtr);
            }
            (*texPtr) = 0;
            (*reload) = true;
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Warning,
                "TextureManager: '%s' exists already! [id=%d]\n",
                name.c_str(), (*texPtr)
            );
            return false;
        }
    }

    return true;
}

bool TextureManager::uploadImage(sgct_core::Image* imgPtr, unsigned int* texPtr) {
    glGenTextures(1, texPtr);
    glBindTexture(GL_TEXTURE_2D, *texPtr);

    bool isBGR = imgPtr->getPreferBGRImport();

    //if three channels
    int textureType = isBGR ? GL_BGR : GL_RGB;

    //if OpenGL 1-2
    if (Engine::instance()->isOGLPipelineFixed()) {
        if (imgPtr->getChannels() == 4) {
            textureType = isBGR ? GL_BGRA : GL_RGBA;
        }
        else if (imgPtr->getChannels() == 1) {
            textureType = (mAlphaMode ? GL_ALPHA : GL_LUMINANCE);
        }
        else if (imgPtr->getChannels() == 2) {
            textureType = GL_LUMINANCE_ALPHA;
        }
    }
    else {
        //OpenGL 3+
        if (imgPtr->getChannels() == 4) {
            textureType = isBGR ? GL_BGRA : GL_RGBA;
        }
        else if (imgPtr->getChannels() == 1) {
            textureType = GL_RED;
        }
        else if (imgPtr->getChannels() == 2) {
            textureType = GL_RG;
        }
    }

    GLint internalFormat;
    unsigned int bpc = static_cast<unsigned int>(imgPtr->getBytesPerChannel());

    if (bpc > 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "TextureManager: %d-bit per channel is not supported!\n",
            bpc * 8
        );
        return false;
    }
    else if (bpc == 2) {
        //turn of compression if 16-bit per color
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "TextureManager: Compression is not supported for bit depths higher than "
            "16-bit per channel!\n"
        );
        mCompression = CompressionMode::None;
    }

    switch (imgPtr->getChannels()) {
        case 4:
            if (mCompression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_RGBA8 : GL_RGBA16);
            }
            else if (mCompression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RGBA;
            }
            else {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            }
            break;
        case 3:
            if (mCompression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_RGB8 : GL_RGB16);
            }
            else if (mCompression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RGB;
            }
            else {
                internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            }
            break;
        case 2:
            if (Engine::instance()->isOGLPipelineFixed()) {
                if (mCompression == CompressionMode::None) {
                    internalFormat = bpc == 1 ?
                        GL_LUMINANCE8_ALPHA8 :
                        GL_LUMINANCE16_ALPHA16;
                }
                else {
                    internalFormat = GL_COMPRESSED_LUMINANCE_ALPHA;
                }
            }
            else {
                if (mCompression == CompressionMode::None) {
                    internalFormat = (bpc == 1 ? GL_RG8 : GL_RG16);
                }
                else if (mCompression == CompressionMode::Generic) {
                    internalFormat = GL_COMPRESSED_RG;
                }
                else {
                    internalFormat = GL_COMPRESSED_RG_RGTC2;
                }
            }
            break;
        case 1:
            if (Engine::instance()->isOGLPipelineFixed()) {
                if (bpc == 1) {
                    internalFormat = (mCompression == CompressionMode::None) ?
                        (mAlphaMode ? GL_ALPHA8 : GL_LUMINANCE8) :
                        (mAlphaMode ? GL_COMPRESSED_ALPHA : GL_COMPRESSED_LUMINANCE);
                }
                else {
                    internalFormat = (mAlphaMode ? GL_ALPHA16 : GL_LUMINANCE16);
                }
            }
            else {
                if (mCompression == CompressionMode::None) {
                    internalFormat = (bpc == 1 ? GL_R8 : GL_R16);
                }
                else if (mCompression == CompressionMode::Generic) {
                    internalFormat = GL_COMPRESSED_RED;
                }
                else {
                    internalFormat = GL_COMPRESSED_RED_RGTC1;
                }
            }
            break;
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "TextureManager: Creating texture... size: %dx%d, %d-channels, compression: %s, "
        "Type: %#04x, Format: %#04x\n",
        imgPtr->getWidth(),
        imgPtr->getHeight(),
        imgPtr->getChannels(),
        (mCompression == CompressionMode::None) ?
            "none" :
            ((mCompression == CompressionMode::Generic) ? "generic" : "S3TC/DXT"),
        textureType,
        internalFormat
    );

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (mMipmapLevels <= 1) {
        mMipmapLevels = 1;
    }

    GLenum format = (bpc == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        static_cast<GLsizei>(imgPtr->getWidth()),
        static_cast<GLsizei>(imgPtr->getHeight()),
        0,
        textureType, 
        format,
        imgPtr->getData()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mMipmapLevels - 1);

    if (mMipmapLevels > 1) {
        glGenerateMipmap(GL_TEXTURE_2D); //allocate the mipmaps

        GLfloat maxAni;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni);
        //MessageHandler::instance()->print("Max anisotropy: %f\n", maxAni);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            mInterpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            mInterpolate ? GL_LINEAR : GL_NEAREST
        );
        glTexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAX_ANISOTROPY_EXT,
            mAnisotropicFilterSize > maxAni ? maxAni : mAnisotropicFilterSize
        );
    }
    else {
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            mInterpolate ? GL_LINEAR : GL_NEAREST
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            mInterpolate ? GL_LINEAR : GL_NEAREST
        );
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWarpMode[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWarpMode[1]);

    return true;
}

void TextureManager::freeTextureData() {
    //the textures might not be stored in a sequence so
    //let's erase them one by one
    for (const std::pair<const std::string, TextureData>& p : mTextures) {
        if (p.second.mId) {
            glDeleteTextures(1, &p.second.mId);
        }
    }
     mTextures.clear();
}

} // namespace sgct_core
