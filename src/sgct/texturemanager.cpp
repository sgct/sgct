/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/texturemanager.h>

#include <sgct/image.h>
#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>

namespace {
    unsigned int uploadImage(const sgct::core::Image& img, bool interpolate, int mipmap,
                             float anisotropicFilterSize)
    {
        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        // if three channels
        GLenum type = GL_BGR;
        if (img.getChannels() == 1) {
            type = GL_RED;
        }
        else if (img.getChannels() == 2) {
            type = GL_RG;
        }
        else if (img.getChannels() == 4) {
            type = GL_BGRA;
        }

        GLenum internalFormat = {};
        switch (img.getChannels()) {
            case 1:
                internalFormat = GL_R8;
                break;
            case 2:
                internalFormat = GL_RG8;
                break;
            case 3:
                internalFormat = GL_RGB8;
                break;
            case 4:
                internalFormat = GL_RGBA8;
                break;
            default:
                throw std::logic_error("Unhandled case label");
        }

        sgct::Logger::Debug(
            "Creating texture. Size: %dx%d, %d-channels, Type: %#04x, Format: %#04x",
            img.getSize().x, img.getSize().y, img.getChannels(), type, internalFormat
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
            type,
            format,
            img.getData()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap - 1);

        if (mipmap > 1) {
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
                anisotropicFilterSize
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
} // namespace

namespace sgct {

TextureManager* TextureManager::_instance = nullptr;

TextureManager& TextureManager::instance() {
    if (!_instance) {
        _instance = new TextureManager;
    }
    return *_instance;
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

unsigned int TextureManager::loadTexture(const std::string& filename, bool interpolate,
                                         float anisotropicFilterSize, int mipmapLevels)
{
    // load image
    core::Image img;
    img.load(filename);
    
    if (img.getData() == nullptr) {
        // image data not valid
        return 0;
    }

    GLuint t = uploadImage(img, interpolate, mipmapLevels, anisotropicFilterSize);
    _textures.push_back(t);

    Logger::Debug("Texture created from '%s' [id=%d]", filename.c_str(), t);
    return t;
}

} // namespace sgct
