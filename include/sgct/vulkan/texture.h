/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_TEXTURE__H__
#define __SGCT__VULKAN_TEXTURE__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

namespace sgct {

class Image;

namespace vulkan {

/**
 * A 2D texture (VkImage + VkImageView + VkSampler) uploaded from host pixel data through a
 * one-time staging buffer. The image is left in SHADER_READ_ONLY_OPTIMAL layout so it can be
 * sampled directly.
 */
class SGCT_EXPORT Texture {
public:
    /**
     * \param context The shared Vulkan context
     * \param width The texture width in pixels
     * \param height The texture height in pixels
     * \param format The pixel format (must match \p data)
     * \param data The host pixel data (tightly packed, no row padding)
     * \param filter The minification and magnification filter of the sampler
     * \param mipLevels The number of mipmap levels that are generated from the pixel
     *        data, which is limited by the size of the texture. With more than one
     *        level, the levels are interpolated linearly. If the format cannot be
     *        filtered while blitting, no mipmaps are generated
     *
     * \throw sgct::Error If a Vulkan call fails or the format is unsupported
     */
    Texture(Context& context, uint32_t width, uint32_t height, VkFormat format,
        const uint8_t* data, VkFilter filter = VK_FILTER_LINEAR, uint32_t mipLevels = 1);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    /**
     * Creates a texture from an 8-bit image as loaded by Image::load. The pixels are
     * expanded to four channels: gray (and gray-alpha) images are replicated into the
     * color channels, and images without alpha are opaque. As Image::load flips the
     * image vertically, the first row of the texture is the bottom of the image, which
     * matches the OpenGL convention of texture coordinates with a bottom-left origin.
     *
     * \param context The shared Vulkan context
     * \param image The image to upload
     *
     * \throw sgct::Error If the image is not an 8-bit image with 1 to 4 channels
     */
    static std::unique_ptr<Texture> fromImage(Context& context, const Image& image);

    VkImage image() const { return _image; }
    VkImageView imageView() const { return _imageView; }
    VkSampler sampler() const { return _sampler; }
    uint32_t width() const { return _width; }
    uint32_t height() const { return _height; }
    VkFormat format() const { return _format; }
    uint32_t mipLevels() const { return _mipLevels; }

private:
    static uint32_t bytesPerPixel(VkFormat format);

    Context& _context;
    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    uint32_t _width = 0;
    uint32_t _height = 0;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    uint32_t _mipLevels = 1;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_TEXTURE__H__
