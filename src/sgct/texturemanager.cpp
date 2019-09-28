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

TextureManager* TextureManager::mInstance = nullptr;

TextureManager* TextureManager::instance() {
    if (mInstance == nullptr) {
        mInstance = new TextureManager();
    }

    return mInstance;
}

void TextureManager::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

TextureManager::TextureManager() {
    setAnisotropicFilterSize(1.f);
    mWarpMode.s = GL_CLAMP_TO_EDGE;
    mWarpMode.t = GL_CLAMP_TO_EDGE;

    TextureData tmpTexture;
    mTextures["NOTSET"] = tmpTexture;
}

TextureManager::~TextureManager() {
    freeTextureData();
}

unsigned int TextureManager::getTextureId(const std::string& name) {
    return mTextures.count(name) > 0 ? mTextures[name].mId : 0;
}

std::string TextureManager::getTexturePath(const std::string& name) {
    return mTextures.count(name) > 0 ? mTextures[name].mPath : "NOT_FOUND";
}

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

void TextureManager::setAlphaModeForSingleChannelTextures(bool alpha) {
    mAlphaMode = alpha;
}

void TextureManager::setOverWriteMode(bool mode) {
    mOverWriteMode = mode;
}

void TextureManager::setAnisotropicFilterSize(float fval) {
    float maximumAnistropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnistropy);

    if (fval >= 1.f && fval <= maximumAnistropy) {
        mAnisotropicFilterSize = fval;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "TextureManager warning: Anisotropic filtersize=%.2f is incorrect.\nMax and "
            "min values for your hardware is %.1f and 1.0\n", maximumAnistropy
        );
    }
}

void TextureManager::setCompression(CompressionMode cm) {
    mCompression = cm;
}

void TextureManager::setWarpingMode(int warpS, int warpT) {
    mWarpMode.s = warpS;
    mWarpMode.t = warpT;
}

TextureManager::CompressionMode TextureManager::getCompression() const {
    return mCompression;
}

bool TextureManager::loadTexture(const std::string& name, const std::string& filename,
                                 bool interpolate, int mipmapLevels)
{
    mInterpolate = interpolate;
    mMipmapLevels = mipmapLevels;

    GLuint texID = 0;
    bool reload = false;
    if (!updateTexture(name, texID, reload)) {
        return true;
    }
    
    std::unordered_map<std::string, TextureData>::iterator textureItem = mTextures.end();

    // load image
    core::Image img;
    if (!img.load(filename)) {
        if (reload) {
            textureItem->second = TextureData();
        }
        return false;
    }
    
    if (img.getData() != nullptr) {
        if (!uploadImage(img, texID)) {
            return false;
        }

        TextureData tmpTexture;
        tmpTexture.mId = texID;
        tmpTexture.mPath.assign(filename);
        tmpTexture.mWidth = static_cast<int>(img.getWidth());
        tmpTexture.mHeight = static_cast<int>(img.getHeight());
        tmpTexture.mChannels = static_cast<int>(img.getChannels());

        if (!reload) {
            mTextures[name] = std::move(tmpTexture);
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

bool TextureManager::loadTexture(const std::string& name, core::Image* imgPtr,
                                 bool interpolate, int mipmapLevels)
{
    if (!imgPtr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "TextureManager: Cannot create texture '%s' from invalid image\n",
            name.c_str()
        );
        return false;
    }
    
    mInterpolate = interpolate;
    mMipmapLevels = mipmapLevels;

    GLuint texID = 0;
    bool reload = false;
    if (!updateTexture(name, texID, reload)) {
        return true;
    }

    if (imgPtr->getData() != nullptr) {
        if (!uploadImage(*imgPtr, texID)) {
            return false;
        }

        TextureData tmpTexture;
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
        // image data not valid
        return false;
    }

    return true;
}

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
    
    // load image
    core::Image img;
    if (!img.load(filename)) {
        return false;
    }

    if (img.getData() != nullptr) {
        if (!uploadImage(img, tmpTexID)) {
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

bool TextureManager::updateTexture(const std::string& name, unsigned int& texPtr,
                                   bool& reload)
{
    // check if texture exits in manager
    bool exist = mTextures.count(name) > 0;
    std::unordered_map<std::string, TextureData>::iterator textureItem = mTextures.end();

    if (exist) {
        // get it
        textureItem = mTextures.find(name);
        texPtr = textureItem->second.mId;

        if (mOverWriteMode) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "TextureManager: Reloading texture '%s'! [id=%d]\n", name.c_str(), texPtr
            );

            if (texPtr != 0) {
                glDeleteTextures(1, &texPtr);
            }
            texPtr = 0;
            reload = true;
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Warning,
                "TextureManager: '%s' exists already! [id=%d]\n", name.c_str(), texPtr
            );
            return false;
        }
    }

    return true;
}

bool TextureManager::uploadImage(const core::Image& imgPtr, unsigned int& texPtr) {
    glGenTextures(1, &texPtr);
    glBindTexture(GL_TEXTURE_2D, texPtr);

    bool isBGR = imgPtr.getPreferBGRImport();

    // if three channels
    int textureType = isBGR ? GL_BGR : GL_RGB;

    if (imgPtr.getChannels() == 4) {
        textureType = isBGR ? GL_BGRA : GL_RGBA;
    }
    else if (imgPtr.getChannels() == 1) {
        textureType = GL_RED;
    }
    else if (imgPtr.getChannels() == 2) {
        textureType = GL_RG;
    }

    GLint internalFormat = 0;
    unsigned int bpc = static_cast<unsigned int>(imgPtr.getBytesPerChannel());

    if (bpc > 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "TextureManager: %d-bit per channel is not supported\n",
            bpc * 8
        );
        return false;
    }
    else if (bpc == 2) {
        // turn of compression if 16-bit per color
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "TextureManager: Compression is not supported for bit depths higher than "
            "16-bit per channel\n"
        );
        mCompression = CompressionMode::None;
    }

    switch (imgPtr.getChannels()) {
        case 1:
            if (mCompression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_R8 : GL_R16);
            }
            else if (mCompression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RED;
            }
            else {
                internalFormat = GL_COMPRESSED_RED_RGTC1;
            }
            break;
        case 2:
            if (mCompression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_RG8 : GL_RG16);
            }
            else if (mCompression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RG;
            }
            else {
                internalFormat = GL_COMPRESSED_RG_RGTC2;
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
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "TextureManager: Creating texture... size: %dx%d, %d-channels, compression: %s, "
        "Type: %#04x, Format: %#04x\n",
        imgPtr.getWidth(),
        imgPtr.getHeight(),
        imgPtr.getChannels(),
        (mCompression == CompressionMode::None) ?
            "none" :
            ((mCompression == CompressionMode::Generic) ? "generic" : "S3TC/DXT"),
        textureType,
        internalFormat
    );

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    mMipmapLevels = glm::max(mMipmapLevels, 1);

    GLenum format = (bpc == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        static_cast<GLsizei>(imgPtr.getWidth()),
        static_cast<GLsizei>(imgPtr.getHeight()),
        0,
        textureType, 
        format,
        imgPtr.getData()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mMipmapLevels - 1);

    if (mMipmapLevels > 1) {
        glGenerateMipmap(GL_TEXTURE_2D); // allocate the mipmaps

        GLfloat maxAni;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni);
        // MessageHandler::instance()->print("Max anisotropy: %f\n", maxAni);

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWarpMode.s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWarpMode.t);

    return true;
}

void TextureManager::freeTextureData() {
    // the textures might not be stored in a sequence so
    // let's erase them one by one
    for (const std::pair<const std::string, TextureData>& p : mTextures) {
        if (p.second.mId) {
            glDeleteTextures(1, &p.second.mId);
        }
    }
     mTextures.clear();
}

} // namespace sgct::core
