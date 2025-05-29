/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/texturemanager.h>

#include <sgct/format.h>
#include <sgct/image.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <algorithm>

namespace {
    unsigned int uploadImage(const sgct::Image& img, bool interpolate, int mipmap,
                             float anisotropicFilterSize)
    {
        unsigned int tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        const auto [type, internalFormat] = [](int c) -> std::pair<GLenum, GLenum> {
            switch (c) {
                case 1: return { GL_RED, GL_R8 };
                case 2: return { GL_RG, GL_RG8 };
                case 3: return { GL_BGR, GL_RGB8 };
                case 4: return { GL_BGRA, GL_RGBA8 };
                default: throw std::logic_error("Unhandled case label");
            }
        }(img.channels());

        sgct::Log::Debug(std::format(
            "Creating texture. Size: {}x{}, {}-channels, Type: {:#04x}, Format: {:#04x}",
            img.size().x, img.size().y, img.channels(), type, internalFormat
        ));

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        constexpr GLenum Format = GL_UNSIGNED_BYTE;
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            img.size().x,
            img.size().y,
            0,
            type,
            Format,
            img.data()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap - 1);

        if (mipmap > 1) {
            glGenerateMipmap(GL_TEXTURE_2D);

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

            // GL_TEXTURE_MAX_ANISOTROPY is no longer an extension, but a core feature in OpenGL 4.0
            #ifdef GL_TEXTURE_MAX_ANISOTROPY
                glTexParameterf(
                    GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY,
                    anisotropicFilterSize
                );
            #else
                glTexParameterf(
                    GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    anisotropicFilterSize
                );
            #endif
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
    glDeleteTextures(static_cast<GLsizei>(_textures.size()), _textures.data());
}

unsigned int TextureManager::loadTexture(const std::filesystem::path& filename,
                                         bool interpolate, float anisotropicFilterSize,
                                         int mipmapLevels)
{
    // load image
    Image img;
    img.load(filename);

    if (img.data() == nullptr) {
        // image data not valid
        return 0;
    }

    unsigned int t = loadTexture(
        img,
        interpolate,
        anisotropicFilterSize,
        mipmapLevels
    );
    Log::Debug(std::format("Texture created from '{}' [id={}]", filename.string(), t));
    return t;
}

unsigned int TextureManager::loadTexture(const Image& img, bool interpolate,
                                         float anisotropicFilterSize, int mipmapLevels)
{
    const GLuint t = uploadImage(img, interpolate, mipmapLevels, anisotropicFilterSize);
    _textures.push_back(t);

    return t;
}

void TextureManager::removeTexture(unsigned int textureId) {
    _textures.erase(
        std::remove(_textures.begin(), _textures.end(), textureId),
        _textures.end()
    );

    glDeleteTextures(1, &textureId);
}

} // namespace sgct
