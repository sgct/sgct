/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

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
 *   anywhere using its static instance. Currently only PNG textures are supported.
 */
class TextureManager {
public:
    /**
     * The compression mode modes. For more info about texute compression look here:
     * <a href="http://en.wikipedia.org/wiki/S3_Texture_Compression">
     * S3 Texture compression</a>
     */
    enum class CompressionMode {
        None = 0,
        Generic,
        S3TC_DXT
    };

    /// Get the TextureManager instance
    static TextureManager* instance();

    /// Destroy the TextureManager
    static void destroy();

    /**
     * This function gets a texture id by its name.
     *
     * \param name of texture
     * \returns openGL texture id if texture is found otherwise 0.
     */
    unsigned int getTextureId(const std::string& name);

    /// Get the texture path. If not found then "NOT_FOUND" is returned.
    std::string getTexturePath(const std::string& name);

    /**
     * Get the dimensions of a texture by name. If not found all variables will be set to
     * -1.
     */
    void getDimensions(const std::string& name, int& width, int& height,
        int& channels) const;

    /// Sets if a single channel texture should be interpreted as alpha or luminance.
    void setAlphaModeForSingleChannelTextures(bool alpha);

    /// Sets if loading a texture with an existing name should be overwritten or not.
    void setOverWriteMode(bool mode);

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

    /**
     * Set the OpenGL texture warping mode. Can be one of the following:
     *   - GL_CLAMP_TO_EDGE (Default)
     *   - GL_CLAMP_TO_BORDER 
     *   - GL_MIRRORED_REPEAT
     *   - GL_REPEAT
     *
     * \param warpS warping parameter along the s-axis (x-axis) 
     * \param warpT warping parameter along the t-axis (y-axis)
     */
    void setWarpingMode(GLenum warpS, GLenum warpT);

    /// \returns the current compression mode
    CompressionMode getCompression() const;

    /**
     * Load a texture to the TextureManager.
     *
     * \param name the name of the texture
     * \param filename the filename or path to the texture
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
                           this value to 1 or less disables mipmaps
     *
     * \return true if texture loaded successfully
     */
    bool loadTexture(const std::string& name, const std::string& filename,
        bool interpolate, int mipmapLevels = 8);

    /**
     * Load a texture to the TextureManager.
     *
     * \param name the name of the texture
     * \param imgPtr pointer to image object
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
     *                     this value to 1 or less disables mipmaps
     *
     * \return true if texture loaded successfully
     */
    bool loadTexture(const std::string& name, core::Image* imgPtr, bool interpolate,
        int mipmapLevels = 8);

    /**
     * Load a unmanged texture. Note that this type of textures doesn't auto destruct.
     *
     * \param texID the openGL texture id
     * \param filename the filename or path to the texture
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
     *                     this value to 1 or less disables mipmaps
     *
     * \return true if texture loaded successfully
     */
    bool loadUnManagedTexture(unsigned int& texID, const std::string& filename,
        bool interpolate, int mipmapLevels = 8);

private:
    struct TextureData {
        std::string path = "NOTSET";
        unsigned int id = 0;
        int width = -1;
        int height = -1;
        int nChannels = -1;
    };

    TextureManager();

    /// Destroy the TextureManager
    ~TextureManager();

    /// \returns true if texture will be uploaded
    bool updateTexture(const std::string& name, unsigned int& texPtr, bool& reload);
    bool uploadImage(const core::Image& imgPtr, unsigned int& texPtr);
    void freeTextureData();

    static TextureManager* _instance;
    
    float _anisotropicFilterSize;
    CompressionMode _compression = CompressionMode::None;
    bool _alphaMode = false;
    bool _overWriteMode = true;
    bool _interpolate = true;
    std::unordered_map<std::string, TextureData> _textures;
    int _mipmapLevels = 8;
    struct {
        GLenum s;
        GLenum t;
    } _warpMode;
};

} // namespace sgct::core

#endif // __SGCT__TEXTURE_MANAGER__H__
