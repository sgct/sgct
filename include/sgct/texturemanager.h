/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__TEXTURE_MANAGER__H__
#define __SGCT__TEXTURE_MANAGER__H__

#include <sgct/ogl_headers.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace sgct {

namespace core { class Image; }

/**
 * The TextureManager loads and handles textures. It is a singleton and can be accessed
 * anywhere using its static instance. Currently only PNG textures are supported.
 */
class TextureManager {
public:
    /**
     * The compression mode modes. For more info about texute compression look here:
     * <a href="http://en.wikipedia.org/wiki/S3_Texture_Compression">
     * S3 Texture compression</a>
     */
    enum class CompressionMode { None = 0, Generic, S3TC_DXT };

    /// Get the TextureManager instance
    static TextureManager* instance();

    /// Destroy the TextureManager
    static void destroy();

    /**
     * Load a texture to the TextureManager.
     *
     * \param filename the filename or path to the texture
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
                           this value to 1 or less disables mipmaps
     * \param compression The compression method that is used for this texture
     * \param anisotropicFilterSize The filter size that is used for the anisotropic
     *        filtering. If this value is 1.f, only bilinear filtering is used
     * \return true The OpenGL name for the texture that was loaded
     */
    unsigned int loadTexture(const std::string& filename, bool interpolate,
        int mipmapLevels = 8, CompressionMode compression = CompressionMode::None,
        float anisotropicFilterSize = 1.f);

private:
    ~TextureManager();
    static TextureManager* _instance;
    std::vector<unsigned int> _textures;
};

} // namespace sgct::core

#endif // __SGCT__TEXTURE_MANAGER__H__
