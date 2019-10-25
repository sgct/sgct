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

namespace sgct::core { class Image; }

namespace sgct {

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
     * Sets the anisotropic filter size. Default is 1.0 (isotropic) which disables
     * anisotropic filtering. This filtering mode can slow down performace. For more info
     * look at:
     * <a href="http://en.wikipedia.org/wiki/Anisotropic_filtering">
     * Anisotropic filtering</a>
     */
    void setAnisotropicFilterSize(float fval);

    /**
     * Set texture compression. Can be one of the following:
     *   - sgct::TextureManager::None
     *   - sgct::TextureManager::Generic
     *   - sgct::TextureManager::S3TC_DXT
     *
     * \param cm the compression mode
     */
    void setCompression(CompressionMode cm);

    /// \return the current compression mode
    CompressionMode getCompression() const;

    /**
     * Load a texture to the TextureManager.
     *
     * \param filename the filename or path to the texture
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
                           this value to 1 or less disables mipmaps
     * \return true if texture loaded successfully
     */
    unsigned int loadTexture(const std::string& filename, bool interpolate,
        int mipmapLevels = 8);

    /**
     * Load a unmanaged texture. Note that this type of textures doesn't auto destruct.
     *
     * \param filename the filename or path to the texture
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
     *                     this value to 1 or less disables mipmaps
     * \return texID the OpenGL texture id
     */
    // unsigned int loadUnManagedTexture(const std::string& filename, bool interpolate,
        // int mipmapLevels = 8);

private:
    ~TextureManager();

    /// \return true if texture will be uploaded
    unsigned int uploadImage(const core::Image& img, bool interpolate, int mipmapLevels);

    static TextureManager* _instance;
    
    float _anisotropicFilterSize = 1.f;;
    CompressionMode _compression = CompressionMode::None;
    std::vector<unsigned int> _textures;
};

} // namespace sgct::core

#endif // __SGCT__TEXTURE_MANAGER__H__
