/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/texturemanager.h>

#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/messagehandler.h>

namespace sgct {

TextureManager* TextureManager::_instance = nullptr;

TextureManager* TextureManager::instance() {
    if (_instance == nullptr) {
        _instance = new TextureManager();
    }

    return _instance;
}

void TextureManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

TextureManager::~TextureManager() {
    for (unsigned int id : _textures) {
        glDeleteTextures(1, &id);
    }
}

void TextureManager::setAnisotropicFilterSize(float fval) {
    float maximumAnistropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnistropy);

    if (fval >= 1.f && fval <= maximumAnistropy) {
        _anisotropicFilterSize = fval;
    }
    else {
        MessageHandler::printWarning(
            "TextureManager: Anisotropic filtersize=%.2f is incorrect. Max and "
            "min values for this hardware is %.1f and 1.0", maximumAnistropy
        );
    }
}

void TextureManager::setCompression(CompressionMode cm) {
    _compression = cm;
}

TextureManager::CompressionMode TextureManager::getCompression() const {
    return _compression;
}

unsigned int TextureManager::loadTexture(const std::string& filename, bool interpolate,
                                         int mipmapLevels)
{
    GLuint texID = 0;

    // load image
    core::Image img;
    if (!img.load(filename)) {
        return 0;
    }
    
    if (img.getData() != nullptr) {
        texID = uploadImage(img, interpolate, mipmapLevels);
        _textures.push_back(texID);

        MessageHandler::printDebug(
            "Texture created from '%s' [id=%d]", filename.c_str(), texID
        );
    }
    else {
        // image data not valid
        return 0;
    }

    return texID;
}

// unsigned int TextureManager::loadUnManagedTexture(const std::string& filename,
//                                                   bool interpolate, int mipmapLevels)
// {
//     core::Image img;
//     if (!img.load(filename)) {
//         return 0;
//     }

//     if (img.getData() == nullptr) {
//         return 0;
//     }

//     unsigned int id = uploadImage(img, interpolate, mipmapLevels);
//     MessageHandler::printDebug(
//         "Unmanaged texture created from '%s' [id=%d]", filename.c_str(), id
//     );
//     return id;
// }

unsigned int TextureManager::uploadImage(const core::Image& img, bool interpolate,
                                         int mipmapLevels)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // if three channels
    GLenum textureType = GL_BGR;

    if (img.getChannels() == 1) {
        textureType = GL_RED;
    }
    else if (img.getChannels() == 2) {
        textureType = GL_RG;
    }
    else if (img.getChannels() == 4) {
        textureType = GL_BGRA;
    }

    GLenum internalFormat = {};
    switch (img.getChannels()) {
        case 1:
            if (_compression == CompressionMode::None) {
                internalFormat = GL_R8;
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
                internalFormat = GL_RG8;
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
                internalFormat = GL_RGB8;
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
                internalFormat = GL_RGBA8;
            }
            else if (_compression == CompressionMode::Generic) {
                internalFormat = GL_COMPRESSED_RGBA;
            }
            else {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            }
            break;
    }

    MessageHandler::printDebug(
        "TextureManager: Creating texture... size: %dx%d, %d-channels, compression: %s, "
        "Type: %#04x, Format: %#04x",
        img.getSize().x, img.getSize().y, img.getChannels(),
        (_compression == CompressionMode::None) ?
            "none" :
            ((_compression == CompressionMode::Generic) ? "generic" : "S3TC/DXT"),
        textureType, internalFormat
    );

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum format = GL_UNSIGNED_BYTE;
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        static_cast<GLsizei>(img.getSize().x),
        static_cast<GLsizei>(img.getSize().y),
        0,
        textureType,
        format,
        img.getData()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);

    if (mipmapLevels > 1) {
        glGenerateMipmap(GL_TEXTURE_2D); // allocate the mipmaps

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            interpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            interpolate ? GL_LINEAR : GL_NEAREST
        );
        glTexParameterf(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAX_ANISOTROPY_EXT,
            _anisotropicFilterSize
        );
    }
    else {
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            interpolate ? GL_LINEAR : GL_NEAREST
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            interpolate ? GL_LINEAR : GL_NEAREST
        );
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return tex;
}

} // namespace sgct::core
