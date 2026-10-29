/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/font.h>

#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/texture.h>
#include <freetype/ftglyph.h>
#include <freetype/ftstroke.h>
#include <algorithm>
#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace {
    struct GlyphData {
        FT_Glyph glyph = nullptr;
        FT_Glyph strokeGlyph = nullptr;
        FT_Stroker stroker = nullptr;
        FT_BitmapGlyph bitmapGlyph = nullptr;
        FT_BitmapGlyph bitmapStrokeGlyph = nullptr;
        FT_Bitmap* bitmap = nullptr;
        FT_Bitmap* strokeBitmap = nullptr;
    };

    struct PixelDataResult {
        bool success = false;
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
        GlyphData gd;
    };

    PixelDataResult getPixelData(FT_Library library, FT_Face face, FT_Fixed strokeSize) {
        PixelDataResult res;

        // Move the face's glyph into a Glyph object
        const FT_Error glyphErr = FT_Get_Glyph(face->glyph, &res.gd.glyph);
        const FT_Error strokeErr = FT_Get_Glyph(face->glyph, &res.gd.strokeGlyph);
        if (glyphErr || strokeErr) {
            res.success = false;
            return res;
        }

        res.gd.stroker = nullptr;
        const FT_Error error = FT_Stroker_New(library, &res.gd.stroker);
        if (!error) {
            FT_Stroker_Set(
                res.gd.stroker,
                64 * strokeSize,
                FT_STROKER_LINECAP_ROUND,
                FT_STROKER_LINEJOIN_ROUND,
                0
            );

            FT_Glyph_Stroke(&res.gd.strokeGlyph, res.gd.stroker, 1);
        }

        // Convert the glyph to a bitmap
        FT_Glyph_To_Bitmap(&res.gd.glyph, ft_render_mode_normal, nullptr, 1);
        res.gd.bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(res.gd.glyph);

        FT_Glyph_To_Bitmap(&res.gd.strokeGlyph, ft_render_mode_normal, nullptr, 1);
        res.gd.bitmapStrokeGlyph = reinterpret_cast<FT_BitmapGlyph>(res.gd.strokeGlyph);

        // This pointer will make accessing the bitmap easier
        res.gd.bitmap = &res.gd.bitmapGlyph->bitmap;
        res.gd.strokeBitmap = &res.gd.bitmapStrokeGlyph->bitmap;

        // Use our helper function to get the widths of the bitmap data that we will need
        // in order to create our texture
        res.width = res.gd.strokeBitmap->width;
        res.height = res.gd.strokeBitmap->rows;

        // Allocate memory for the texture data
        res.pixels.resize(
            2 * static_cast<size_t>(res.width) * static_cast<size_t>(res.height)
        );
        std::fill(res.pixels.begin(), res.pixels.end(), static_cast<unsigned char>(0));

        // Read alpha to one channel and stroke - alpha in the second channel. We use the
        // ?: operator so that value which we use will be 0 if we are in the padding zone,
        // and whatever is the the Freetype bitmap otherwise
        const int offsetWidth = (res.gd.strokeBitmap->width - res.gd.bitmap->width) / 2;
        const int offsetRows = (res.gd.strokeBitmap->rows - res.gd.bitmap->rows) / 2;
        for (int j = 0; j < res.height; j++) {
            for (int i = 0; i < res.width; i++) {
                const int k = i - offsetWidth;
                const int l = j - offsetRows;


                const size_t idx = 2 * (
                    static_cast<size_t>(i) + static_cast<size_t>(j) *
                    static_cast<size_t>(res.width)
                );
                if (k >= static_cast<int>(res.gd.bitmap->width) ||
                    l >= static_cast<int>(res.gd.bitmap->rows) || k < 0 || l < 0)
                {
                    res.pixels[idx] = 0;
                }
                else {
                    res.pixels[idx] = res.gd.bitmap->buffer[k + res.gd.bitmap->width * l];
                }

                const bool strokeInRange =
                    i >= static_cast<int>(res.gd.strokeBitmap->width) ||
                    j >= static_cast<int>(res.gd.strokeBitmap->rows);
                const unsigned char stroke =
                    strokeInRange ?
                    0 :
                    res.gd.strokeBitmap->buffer[i + res.gd.strokeBitmap->width * j];

                res.pixels[idx + 1] = stroke < res.pixels[idx] ?
                    res.pixels[idx] :
                    stroke;
            }
        }

        res.success = true;
        return res;
    }

    // The glyphs are packed into rows of the atlas with this many pixels between them, so
    // that sampling at the edge of a glyph does not pick up its neighbor
    constexpr int AtlasPadding = 1;
    constexpr int MaxAtlasWidth = 1024;
} // namespace

namespace sgct::text {

Font::Font(FT_Library lib, FT_Face face, unsigned int height)
    : _library(lib)
    , _face(face)
    , _height(static_cast<float>(height))
{
    createAtlas();
}

Font::~Font() {
    _atlas = nullptr;
    FT_Done_Face(_face);
}

void Font::setStrokeSize(int size) {
    if (size == _strokeSize) {
        return;
    }
    _strokeSize = size;

    // The atlas might still be used by frames that are in flight
    vkDeviceWaitIdle(vulkan::Context::instance().device());
    createAtlas();
}

const Font::FontFaceData& Font::fontFaceData(char c) const {
    return _fontFaceData[static_cast<unsigned char>(c)];
}

const vulkan::Texture& Font::atlas() const {
    return *_atlas;
}

float Font::height() const {
    return _height;
}

void Font::createAtlas() {
    struct Glyph {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
    };
    std::array<Glyph, 256> glyphs;

    // Rasterize all glyphs that the font contains
    for (size_t i = 0; i < glyphs.size(); i++) {
        const char c = static_cast<char>(i);
        FontFaceData& ffd = _fontFaceData[i];
        ffd = FontFaceData();

        // Load the Glyph for our character.
        // Hints:
        // www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX
        const FT_UInt charIndex = FT_Get_Char_Index(_face, static_cast<FT_ULong>(c));
        if (charIndex == 0) {
            continue;
        }
        if (FT_Load_Glyph(_face, charIndex, FT_LOAD_FORCE_AUTOHINT)) {
            Log::Error(std::format("Error creating character {}", c));
            continue;
        }

        PixelDataResult res = getPixelData(_library, _face, _strokeSize);
        if (!res.success) {
            Log::Error(std::format("Error creating character {}", c));
            continue;
        }

        // Setup geometry data
        ffd.pos.x = static_cast<float>(res.gd.bitmapGlyph->left);
        // abock (2010-10-19) Don't remove this variable;  if the expression is directly
        // inserted in the static_cast, something goes wrong when rows > top and things
        // get wrongly converted into unsigned integer type before the cast
        const int y = res.gd.bitmapGlyph->top - res.gd.bitmap->rows;
        ffd.pos.y = static_cast<float>(y);
        ffd.size.x = static_cast<float>(res.width);
        ffd.size.y = static_cast<float>(res.height);
        ffd.distToNextChar = static_cast<float>(_face->glyph->advance.x / 64);
        ffd.hasPixels = c != ' ' && res.width > 0 && res.height > 0;

        if (ffd.hasPixels) {
            glyphs[i].width = res.width;
            glyphs[i].height = res.height;
            glyphs[i].pixels = std::move(res.pixels);
        }

        FT_Stroker_Done(res.gd.stroker);
        FT_Done_Glyph(res.gd.strokeGlyph);
        FT_Done_Glyph(res.gd.glyph);
    }

    // Pack the glyphs into rows
    int atlasWidth = 1;
    int penX = 0;
    int penY = 0;
    int rowHeight = 0;
    for (Glyph& g : glyphs) {
        if (g.pixels.empty()) {
            continue;
        }
        if (penX > 0 && penX + g.width > MaxAtlasWidth) {
            penX = 0;
            penY += rowHeight + AtlasPadding;
            rowHeight = 0;
        }
        g.x = penX;
        g.y = penY;
        penX += g.width + AtlasPadding;
        rowHeight = std::max(rowHeight, g.height);
        atlasWidth = std::max(atlasWidth, g.x + g.width);
    }
    const int atlasHeight = std::max(penY + rowHeight, 1);

    // Two channels per pixel: the glyph and the glyph including its stroke
    std::vector<uint8_t> pixels(2 * static_cast<size_t>(atlasWidth) * atlasHeight, 0);
    for (size_t i = 0; i < glyphs.size(); i++) {
        const Glyph& g = glyphs[i];
        if (g.pixels.empty()) {
            continue;
        }
        for (int row = 0; row < g.height; row++) {
            std::copy_n(
                g.pixels.data() + 2 * static_cast<size_t>(row) * g.width,
                2 * g.width,
                pixels.data() + 2 * (static_cast<size_t>(g.y + row) * atlasWidth + g.x)
            );
        }

        const float w = static_cast<float>(atlasWidth);
        const float h = static_cast<float>(atlasHeight);
        _fontFaceData[i].texRect = vec4{
            g.x / w,
            g.y / h,
            (g.x + g.width) / w,
            (g.y + g.height) / h
        };
    }

    _atlas = std::make_unique<vulkan::Texture>(
        vulkan::Context::instance(),
        static_cast<uint32_t>(atlasWidth),
        static_cast<uint32_t>(atlasHeight),
        VK_FORMAT_R8G8_UNORM,
        pixels.data(),
        VK_FILTER_NEAREST
    );
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
