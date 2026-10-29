/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FONTMANAGER__H__
#define __SGCT__FONTMANAGER__H__

#ifdef SGCT_HAS_TEXT

#include <sgct/sgctexports.h>

#include <sgct/font.h>
#include <sgct/math.h>
#include <sgct/vulkan/pipeline.h>
#include <vulkan/vulkan.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

typedef struct FT_LibraryRec_* FT_Library;

namespace sgct::text {

class Font;

/**
 * Singleton for font handling. A lot of the font handling is based on Nehe's tutorial
 * for freetype [link](http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/)
 *
 * How to load a font (after the Engine was initialized, as the fonts create Vulkan
 * textures):
 *
 * ```cpp
 * // Add Verdana size 14 to the FontManager using the system font path
 * if (!sgct::text::FontManager::instance().addFont("Verdana", "verdana.ttf"))
 *    sgct::text::FontManager::instance().getFont("Verdana", 14);
 *
 * // Add Special font from local path
 * if (!sgct::text::FontManager::instance().addFont(
 *       "Special",
 *       "Special.ttf",
 *       sgct::text::FontManager::Local
 *  ))
 * {
 *   sgct::text::FontManager::instance().getFont("Special", 14);
 * }
 * ```
 *
 * Then in the draw or draw2d callback the font can be rendered:
 * ```cpp
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getFont("Verdana", 14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     "Hello World"
 * );
 * ```
 *
 * SGCT has an internal font that can be used as well:
 * ```cpp
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     "Hello World"
 * );
 * ```
 *
 * Non ASCII characters are supported as well:
 * ```cpp
 * sgct::text::print(
 *     sgct::text::FontManager::instance().getDefaultFont(14),
 *     sgct::text::TopLeft,
 *     50,
 *     50,
 *     L"Hallå Världen"
 * );
 * ```
 */
class SGCT_EXPORT FontManager {
public:
    static FontManager& instance();
    static void destroy();

    /**
     * Destructor cleans up all font objects, textures and pipelines. The GPU must not be
     * using them anymore.
     */
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager(FontManager&&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager& operator=(FontManager&&) = delete;

    /**
     * Adds a font file to the manager.
     *
     * \param name Specify a name for the font
     * \param file Path to the font file
     * \param isAbsolutePath If this value is `true`, the \p file specifies an absolute
     *        path
     */
    bool addFont(std::string name, std::string file, bool isAbsolutePath = false);

    /**
     * Get a font face that is loaded into memory.
     *
     * \param name Name of the font
     * \param height Height in  pixels for the font
     * \return Pointer to the font face, `nullptr` if not found
     */
    Font* font(const std::string& name, unsigned int height = 10);

    /**
     * Binds the font pipeline for scene render passes of \p format (creating it for
     * \p renderPass if needed) and the atlas texture of \p font.
     */
    void bindPipeline(VkCommandBuffer cmd, VkRenderPass renderPass,
        const vulkan::SceneFormat& format, const Font& font);

    /**
     * Records the draw of a single glyph with the font pipeline that has been bound
     * through #bindPipeline.
     *
     * \param cmd The command buffer to record into
     * \param mvp The matrix transforming the unit quad of the glyph
     * \param color The color of the glyph
     * \param texRect The area of the glyph in the atlas (see Font::FontFaceData)
     */
    void drawGlyph(VkCommandBuffer cmd, const mat4& mvp, const vec4& color,
        const vec4& texRect) const;

private:
    /**
     * Constructor initiates the freetype library.
     */
    FontManager();

    /**
     * Creates font textures with a specific height if a path to the font exists.
     *
     * \param name Name of the font
     * \param height Height of the font in pixels
     * \return Pointer to the newly created font, nullptr if something went wrong
     */
    std::unique_ptr<Font> createFont(const std::string& name, int height);

    static FontManager* _instance;

    FT_Library _library;

    /// Holds all predefined font paths for generating font glyphs
    std::map<std::string, std::string> _fontPaths;

    /// All generated fonts
    std::map<std::pair<std::string, unsigned int>, std::unique_ptr<Font>> _fontMap;

    VkDescriptorSetLayout _setLayout = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    /// The pipelines for the color formats of the render targets that text is drawn into
    std::map<vulkan::SceneFormat, std::unique_ptr<vulkan::Pipeline>> _pipelines;
};

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
#endif // __SGCT__FONTMANAGER__H__
