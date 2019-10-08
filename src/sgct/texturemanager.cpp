/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/TextureManager.h>

#include <sgct/Engine.h>
#include <sgct/Image.h>
#include <sgct/MessageHandler.h>

namespace sgct {

TextureManager* TextureManager::_instance = nullptr;

TextureManager* TextureManager::instance() {
    if (_instance == nullptr) {
        _instance = new TextureManager();
    }

    return _instance;
}

void TextureManager::destroy() {
    if (_instance != nullptr) {
        delete _instance;
        _instance = nullptr;
    }
}

TextureManager::TextureManager() {
    setAnisotropicFilterSize(1.f);
    _warpMode.s = GL_CLAMP_TO_EDGE;
    _warpMode.t = GL_CLAMP_TO_EDGE;

    TextureData tmpTexture;
    _textures["NOTSET"] = tmpTexture;
}

TextureManager::~TextureManager() {
    freeTextureData();
}

unsigned int TextureManager::getTextureId(const std::string& name) {
    return _textures.count(name) > 0 ? _textures[name].id : 0;
}

std::string TextureManager::getTexturePath(const std::string& name) {
    return _textures.count(name) > 0 ? _textures[name].path : "NOT_FOUND";
}

void TextureManager::getDimensions(const std::string& name, int& width, int& height,
                                   int& channels) const
{
    if (_textures.count(name) > 0) {
        const TextureData& texData = _textures.at(name);
        width = texData.width;
        height = texData.width;
        channels = texData.width;
    }
    else {
        width = -1;
        height = -1;
        channels = -1;
    }
}

void TextureManager::setAlphaModeForSingleChannelTextures(bool alpha) {
    _alphaMode = alpha;
}

void TextureManager::setOverWriteMode(bool mode) {
    _overWriteMode = mode;
}

void TextureManager::setAnisotropicFilterSize(float fval) {
    float maximumAnistropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnistropy);

    if (fval >= 1.f && fval <= maximumAnistropy) {
        _anisotropicFilterSize = fval;
    }
    else {
        MessageHandler::instance()->printWarning(
            "TextureManager warning: Anisotropic filtersize=%.2f is incorrect. Max and "
            "min values for your hardware is %.1f and 1.0", maximumAnistropy
        );
    }
}

void TextureManager::setCompression(CompressionMode cm) {
    _compression = cm;
}

void TextureManager::setWarpingMode(GLenum warpS, GLenum warpT) {
    _warpMode.s = warpS;
    _warpMode.t = warpT;
}

TextureManager::CompressionMode TextureManager::getCompression() const {
    return _compression;
}

bool TextureManager::loadTexture(const std::string& name, const std::string& filename,
                                 bool interpolate, int mipmapLevels)
{
    _interpolate = interpolate;
    _mipmapLevels = mipmapLevels;

    GLuint texID = 0;
    bool reload = false;
    if (!updateTexture(name, texID, reload)) {
        return true;
    }
    
    std::unordered_map<std::string, TextureData>::iterator textureItem = _textures.end();

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
        tmpTexture.id = texID;
        tmpTexture.path = filename;
        tmpTexture.width = static_cast<int>(img.getWidth());
        tmpTexture.height = static_cast<int>(img.getHeight());
        tmpTexture.nChannels = static_cast<int>(img.getChannels());

        if (!reload) {
            _textures[name] = std::move(tmpTexture);
        }

        MessageHandler::instance()->printDebug(
            "TextureManager: Texture created from '%s' [id=%d]", filename.c_str(), texID
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
        MessageHandler::instance()->printDebug(
            "TextureManager: Cannot create texture '%s' from invalid image", name.c_str()
        );
        return false;
    }
    
    _interpolate = interpolate;
    _mipmapLevels = mipmapLevels;

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
        tmpTexture.id = texID;
        tmpTexture.path = "NOTSET";
        tmpTexture.width = static_cast<int>(imgPtr->getWidth());
        tmpTexture.height = static_cast<int>(imgPtr->getHeight());
        tmpTexture.nChannels = static_cast<int>(imgPtr->getChannels());

        if (!reload) {
            _textures[name] = tmpTexture;
        }

        MessageHandler::instance()->printDebug(
            "TextureManager: Texture created from image [id=%d]", texID
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
    _interpolate = interpolate;
    _mipmapLevels = mipmapLevels;

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

        MessageHandler::instance()->printDebug(
            "TextureManager: Unmanaged texture created from '%s' [id=%d]",
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
    bool exist = _textures.count(name) > 0;
    std::unordered_map<std::string, TextureData>::iterator textureItem = _textures.end();

    if (exist) {
        // get it
        textureItem = _textures.find(name);
        texPtr = textureItem->second.id;

        if (_overWriteMode) {
            MessageHandler::instance()->printDebug(
                "TextureManager: Reloading texture '%s'! [id=%d]", name.c_str(), texPtr
            );

            if (texPtr != 0) {
                glDeleteTextures(1, &texPtr);
            }
            texPtr = 0;
            reload = true;
        }
        else {
            MessageHandler::instance()->printWarning(
                "TextureManager: '%s' exists already! [id=%d]", name.c_str(), texPtr
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
    GLenum textureType = isBGR ? GL_BGR : GL_RGB;

    if (imgPtr.getChannels() == 4) {
        textureType = isBGR ? GL_BGRA : GL_RGBA;
    }
    else if (imgPtr.getChannels() == 1) {
        textureType = GL_RED;
    }
    else if (imgPtr.getChannels() == 2) {
        textureType = GL_RG;
    }

    GLenum internalFormat = {};
    unsigned int bpc = static_cast<unsigned int>(imgPtr.getBytesPerChannel());

    if (bpc > 2) {
        MessageHandler::instance()->printError(
            "TextureManager: %d-bit per channel is not supported", bpc * 8
        );
        return false;
    }
    else if (bpc == 2) {
        // turn of compression if 16-bit per color
        MessageHandler::instance()->printWarning(
            "TextureManager: Compression is not supported for bit depths higher than "
            "16-bit per channel"
        );
        _compression = CompressionMode::None;
    }

    switch (imgPtr.getChannels()) {
        case 1:
            if (_compression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_R8 : GL_R16);
            }
            else if (_compression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RED;
            }
            else {
                internalFormat = GL_COMPRESSED_RED_RGTC1;
            }
            break;
        case 2:
            if (_compression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_RG8 : GL_RG16);
            }
            else if (_compression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RG;
            }
            else {
                internalFormat = GL_COMPRESSED_RG_RGTC2;
            }
            break;
        case 3:
            if (_compression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_RGB8 : GL_RGB16);
            }
            else if (_compression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RGB;
            }
            else {
                internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            }
            break;
        case 4:
            if (_compression == CompressionMode::None) {
                internalFormat = (bpc == 1 ? GL_RGBA8 : GL_RGBA16);
            }
            else if (_compression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RGBA;
            }
            else {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            }
            break;
    }

    MessageHandler::instance()->printDebug(
        "TextureManager: Creating texture... size: %dx%d, %d-channels, compression: %s, "
        "Type: %#04x, Format: %#04x",
        imgPtr.getWidth(), imgPtr.getHeight(), imgPtr.getChannels(),
        (_compression == CompressionMode::None) ?
            "none" :
            ((_compression == CompressionMode::Generic) ? "generic" : "S3TC/DXT"),
        textureType, internalFormat
    );

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    _mipmapLevels = glm::max(_mipmapLevels, 1);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, _mipmapLevels - 1);

    if (_mipmapLevels > 1) {
        glGenerateMipmap(GL_TEXTURE_2D); // allocate the mipmaps

        GLfloat maxAni;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni);
        // MessageHandler::instance()->print("Max anisotropy: %f", maxAni);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            _interpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            _interpolate ? GL_LINEAR : GL_NEAREST
        );
        glTexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAX_ANISOTROPY_EXT,
            _anisotropicFilterSize > maxAni ? maxAni : _anisotropicFilterSize
        );
    }
    else {
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            _interpolate ? GL_LINEAR : GL_NEAREST
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            _interpolate ? GL_LINEAR : GL_NEAREST
        );
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _warpMode.s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _warpMode.t);

    return true;
}

void TextureManager::freeTextureData() {
    // the textures might not be stored in a sequence so
    // let's erase them one by one
    for (const std::pair<const std::string, TextureData>& p : _textures) {
        if (p.second.id) {
            glDeleteTextures(1, &p.second.id);
        }
    }
     _textures.clear();
}

} // namespace sgct::core
