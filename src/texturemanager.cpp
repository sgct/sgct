/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/texturemanager.h>

#include <sgct/format.h>
#include <sgct/image.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
    unsigned int uploadImage(const sgct::Image& img, bool interpolate, int mipmap,
                             float anisotropicFilterSize)
    {
        unsigned int tex = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);

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

        glTextureParameteri(tex, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(tex, GL_TEXTURE_MAX_LEVEL, mipmap - 1);

        const GLenum interp = interpolate ? GL_LINEAR : GL_NEAREST;

        if (mipmap > 1) {
            glTextureParameteri(
                tex,
                GL_TEXTURE_MIN_FILTER,
                interpolate ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR
            );
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, interp);
            glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY, anisotropicFilterSize);
        }
        else {
            glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, interp);
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, interp);
        }

        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTextureStorage2D(tex, mipmap, internalFormat, img.size().x, img.size().y);
        glTextureSubImage2D(
            tex,
            0,
            0,
            0,
            img.size().x,
            img.size().y,
            type,
            GL_UNSIGNED_BYTE,
            img.data()
        );

        if (mipmap > 1) {
            glGenerateTextureMipmap(tex);
        }

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
    Log::Debug(std::format("Texture created from '{}' [id={}]", filename, t));
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
