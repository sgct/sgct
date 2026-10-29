/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FONT__H__
#define __SGCT__FONT__H__

#ifdef SGCT_HAS_TEXT

#include <sgct/sgctexports.h>

#include <sgct/math.h>
#include <array>
#include <memory>

typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;
typedef signed long FT_Fixed;

namespace sgct::vulkan { class Texture; }

namespace sgct::text {

/**
 * Will handle font textures and rendering. Implementation is based on
 * <a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehe's font
 * tutorial for freetype</a>. All glyphs of the font are rasterized into a single atlas
 * texture when the font is created, which stores the glyph's coverage in the red channel
 * and the coverage of the glyph including its stroke (border) in the green channel.
 */
class SGCT_EXPORT Font {
public:
    struct FontFaceData {
        float distToNextChar = 0.f;
        /// The offset of the glyph's quad from the pen position in pixels
        vec2 pos = vec2{ 0.f, 0.f };
        /// The size of the glyph's quad in pixels
        vec2 size = vec2{ 0.f, 0.f };
        /// The area of the glyph in the atlas texture as (u0, v0, u1, v1), with v0 being
        /// the top row of the glyph
        vec4 texRect = vec4{ 0.f, 0.f, 0.f, 0.f };
        /// Whether the glyph has any pixels that need to be drawn
        bool hasPixels = false;
    };

    /**
     * Initializes all variables needed for the font and rasterizes its glyphs into the
     * atlas texture. Requires the Vulkan context.
     *
     * \param lib The freetype library
     * \param face The truetype face pointer
     * \param height Font height in pixels
     */
    Font(FT_Library lib, FT_Face face, unsigned int height);

    /**
     * Cleans up memory used by the Font and destroys the atlas texture. The GPU must not
     * be using the atlas anymore.
     */
    ~Font();

    /**
     * Get the font face data. Characters that the font does not contain have no pixels
     * and do not advance the pen.
     */
    const Font::FontFaceData& fontFaceData(char c) const;

    /**
     * The texture containing all glyphs of the font.
     */
    const vulkan::Texture& atlas() const;

    /**
     * Get height of the font.
     */
    float height() const;

    /**
     * Set the stroke (border) size. This rasterizes the glyphs again, so it waits until
     * the GPU is not using the atlas anymore.
     *
     * \param size The stroke size in pixels
     */
    void setStrokeSize(int size);

private:
    void createAtlas();

    const FT_Library _library;
    const FT_Face _face;
    FT_Fixed _strokeSize = 1;
    const float _height;
    std::array<FontFaceData, 256> _fontFaceData;
    std::unique_ptr<vulkan::Texture> _atlas;
};

} // namespace sgct

#endif // SGCT_HAS_TEXT
#endif // __SGCT__FONT__H__
