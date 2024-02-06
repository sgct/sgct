/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__TEXTUREMANAGER__H__
#define __SGCT__TEXTUREMANAGER__H__

#include <sgct/sgctexports.h>
#include <string>
#include <vector>

namespace sgct {

class Image;

/**
 * The TextureManager loads and handles textures. It is a singleton and can be accessed
 * anywhere using its static instance. Currently only PNG textures are supported.
 */
class SGCT_EXPORT TextureManager {
public:
    static TextureManager& instance();
    static void destroy();

    /**
     * Loads a texture to the TextureManager.
     *
     * \param filename The filename or path to the texture
     * \param interpolate Set to true for using interpolation (bi-linear filtering)
     * \param anisotropicFilterSize The filter size that is used for the anisotropic
     *        filtering. If this value is 1.f, only bilinear filtering is used
     * \param mipmapLevels The number of mipmap levels that will be generated, setting
     *        this value to 1 or less disables mipmaps
     * \return The OpenGL name for the texture that was loaded
     */
    unsigned int loadTexture(const std::string& filename, bool interpolate = true,
        float anisotropicFilterSize = 1.f, int mipmapLevels = 8);

    /**
     * Loads a texture to the TextureManager.
     *
     * \param img The image with the texture data
     * \param interpolate Set to true for using interpolation (bi-linear filtering)
     * \param anisotropicFilterSize The filter size that is used for the anisotropic
     *        filtering. If this value is 1.f, only bilinear filtering is used
     * \param mipmapLevels The number of mipmap levels that will be generated, setting
              this value to 1 or less disables mipmaps
     * \return The OpenGL name for the texture that was loaded
     */
    unsigned int loadTexture(const Image& img, bool interpolate = true,
        float anisotropicFilterSize = 1.f, int mipmapLevels = 8);

    /**
     * Removes a previously generated OpenGL texture.
     *
     * \param textureId The id of the texture that should be deleted, this has to be an id
     *        that was returned from a previous call to loadTexture
     */
    void removeTexture(unsigned int textureId);

private:
    ~TextureManager();
    static TextureManager* _instance;
    std::vector<unsigned int> _textures;
};

} // namespace sgct

#endif // __SGCT__TEXTUREMANAGER__H__
