/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/Font.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/ogl_headers.h>
#include <array>

namespace sgct::text {

Font::Font(FT_Library lib, FT_Face face, std::string name, unsigned int height)
    : _name(std::move(name))
    , _height(static_cast<float>(height))
    , _face(face)
    , _library(lib)
{
    // setup geometry
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    MessageHandler::instance()->printDebug("Font: Generating VAO: %u\n", _vao);
    MessageHandler::instance()->printDebug("Font: Generating VBO: %u\n", _vbo);

    std::array<float, 16> c = {
        0.f, 1.f, 0.f, 0.f,
        1.f, 1.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 1.f, 1.f
    };

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, c.size() * sizeof(float), c.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(8)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Font::~Font() {
    if (!_fontFaceData.empty()) {
        glDeleteLists(_listId, 1);
        glDeleteVertexArrays(1, &_vao);
        glDeleteBuffers(1, &_vbo);

        for (const std::pair<const wchar_t, FontFaceData>& n : _fontFaceData) {
            glDeleteTextures(1, &(n.second.texId));
            FT_Done_Glyph(n.second.glyph);
        }
    }

    // This needs to be cleared before the face is destroyed
    _fontFaceData.clear();
    FT_Done_Face(_face);
}

long Font::getStrokeSize() const {
    return _strokeSize;
}

void Font::setStrokeSize(long size) {
    _strokeSize = size;
};

const Font::FontFaceData& Font::getFontFaceData(wchar_t c) {
    if (_fontFaceData.count(c) == 0) {
        // check if c does not exist in map
        createCharacter(c);
    }
    return _fontFaceData[c];
}

unsigned int Font::getVAO() const {
    return _vao;
}

unsigned int Font::getVBO() const {
    return _vbo;
}

unsigned int Font::getDisplayList() const {
    return _listId;
}

float Font::getHeight() const {
    return _height;
}

void Font::createCharacter(wchar_t c) {
    FontFaceData ffd;
    const bool success = createGlyph(c, ffd);
    if (success) {
        _fontFaceData[c] = std::move(ffd);
    }
}

bool Font::createGlyph(wchar_t c, FontFaceData& ffd) {
    // Load the Glyph for our character.
    // Hints:
    // www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX

    FT_UInt charIndex = FT_Get_Char_Index(_face, static_cast<FT_ULong>(c));
    if (charIndex == 0) {
        MessageHandler::instance()->printDebug(
            "Font %s: Missing face for char %u\n",
            _name.c_str(), static_cast<unsigned int>(c)
        );
    }

    FT_Error loadError = FT_Load_Glyph(_face, charIndex, FT_LOAD_FORCE_AUTOHINT);
    if (loadError) {
        MessageHandler::instance()->printError(
            "Font %s: FT_Load_Glyph failed for char %u\n",
            _name.c_str(), static_cast<unsigned int>(c)
        );
        return false;
    }

    // load pixel data
    int width;
    int height;
    std::vector<unsigned char> pixels;
    GlyphData gd;
    const bool success = getPixelData(_face, width, height, pixels, gd);
    if (!success) {
        MessageHandler::instance()->printError(
            "Font %s: FT_Get_Glyph failed for char %u\n",
            _name.c_str(), static_cast<unsigned int>(c)
        );
        return false;
    }

    // create texture
    if (charIndex > 0) {
        ffd.texId = generateTexture(width, height, pixels);
    }
    else {
        ffd.texId = 0;
    }

    // With the texture created, we don't need to expanded data anymore
    pixels.clear();

    // setup geometry data
    ffd.pos.x = static_cast<float>(gd.bitmapGlyph->left);
    ffd.pos.y = static_cast<float>(gd.bitmapGlyph->top - gd.bitmap->rows);
    ffd.size.x = static_cast<float>(width);
    ffd.size.y = static_cast<float>(height);

    // delete the stroke glyph
    FT_Stroker_Done(gd.stroker);
    FT_Done_Glyph(gd.strokeGlyph);
    
    // Can't delete them while they are used, delete when font is cleaned
    ffd.glyph = gd.glyph;
    ffd.distToNextChar = static_cast<float>(_face->glyph->advance.x / 64);

    return true;
}

unsigned int Font::generateTexture(int width, int height,
                                   const std::vector<unsigned char>& buffer)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_COMPRESSED_RG,
        width,
        height,
        0,
        GL_RG,
        GL_UNSIGNED_BYTE,
        buffer.data()
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    return tex;
}

bool Font::getPixelData(FT_Face face, int& width, int& height,
                        std::vector<unsigned char>& pixels, GlyphData& gd)
{
    // Move the face's glyph into a Glyph object
    FT_Error glyphErr = FT_Get_Glyph(face->glyph, &(gd.glyph));
    FT_Error strokeErr = FT_Get_Glyph(face->glyph, &(gd.strokeGlyph));
    if (glyphErr || strokeErr) {
        return false;
    }

    gd.stroker = nullptr;
    FT_Error error = FT_Stroker_New(_library, &(gd.stroker));
    if (!error) {
        FT_Stroker_Set(
            gd.stroker,
            64 * _strokeSize,
            FT_STROKER_LINECAP_ROUND,
            FT_STROKER_LINEJOIN_ROUND,
            0
        );

        error = FT_Glyph_Stroke(&(gd.strokeGlyph), gd.stroker, 1);
    }

    // Convert the glyph to a bitmap
    FT_Glyph_To_Bitmap(&(gd.glyph), ft_render_mode_normal, 0, 1);
    gd.bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(gd.glyph);

    FT_Glyph_To_Bitmap(&(gd.strokeGlyph), ft_render_mode_normal, 0, 1);
    gd.bitmapStrokeGlyph = reinterpret_cast<FT_BitmapGlyph>(gd.strokeGlyph);

    // This pointer will make accessing the bitmap easier
    gd.bitmap = &(gd.bitmapGlyph->bitmap);
    gd.strokeBitmap = &(gd.bitmapStrokeGlyph->bitmap);

    // Use our helper function to get the widths of the bitmap data that we will need in
    // order to create our texture
    width = gd.strokeBitmap->width;
    height = gd.strokeBitmap->rows;

    // Allocate memory for the texture data
    pixels.resize(2 * width * height);
    std::fill(pixels.begin(), pixels.end(), static_cast<unsigned char>(0));

    // read alpha to one channel and stroke - alpha in the second channel. We use the ?:
    // operator so that value which we use will be 0 if we are in the padding zone, and
    // whatever is the the Freetype bitmap otherwise
    const int offsetWidth = (gd.strokeBitmap->width - gd.bitmap->width) / 2;
    const int offsetRows = (gd.strokeBitmap->rows - gd.bitmap->rows) / 2;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const int k = i - offsetWidth;
            const int l = j - offsetRows;

            const int idx = 2 * (i + j * width);
            if (k >= static_cast<int>(gd.bitmap->width) ||
                l >= static_cast<int>(gd.bitmap->rows) || k < 0 || l < 0)
            {
                pixels[idx] = 0;
            }
            else {
                pixels[idx] = gd.bitmap->buffer[k + gd.bitmap->width * l];
            }

            const bool strokeInRange =
                i >= static_cast<int>(gd.strokeBitmap->width) ||
                j >= static_cast<int>(gd.strokeBitmap->rows);
            unsigned char strokeVal = strokeInRange ?
                0 : gd.strokeBitmap->buffer[i + gd.strokeBitmap->width * j];

            pixels[idx + 1] = strokeVal < pixels[idx] ? pixels[idx] : strokeVal;
        }
    }

    return true;
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
