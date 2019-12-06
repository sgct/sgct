/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__TEXTUREMANAGER__H__
#define __SGCT__TEXTUREMANAGER__H__

#include <string>
#include <vector>

namespace sgct {

class Image;

/**
 * The TextureManager loads and handles textures. It is a singleton and can be accessed
 * anywhere using its static instance. Currently only PNG textures are supported.
 */
class TextureManager {
public:
    static TextureManager& instance();
    static void destroy();

    /**
     * Load a texture to the TextureManager.
     *
     * \param filename the filename or path to the texture
     * \param interpolate set to true for using interpolation (bi-linear filtering)
     * \param anisotropicFilterSize The filter size that is used for the anisotropic
     *        filtering. If this value is 1.f, only bilinear filtering is used
     * \param mipmapLevels is the number of mipmap levels that will be generated, setting
              this value to 1 or less disables mipmaps
     * \return The OpenGL name for the texture that was loaded
     */
    unsigned int loadTexture(const std::string& filename, bool interpolate,
        float anisotropicFilterSize = 1.f, int mipmapLevels = 8);

private:
    ~TextureManager();
    static TextureManager* _instance;
    std::vector<unsigned int> _textures;
};

} // namespace sgct

#endif // __SGCT__TEXTUREMANAGER__H__
