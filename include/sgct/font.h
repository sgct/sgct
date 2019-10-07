/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__FREETYPE_FONT__H__
#define __SGCT__FREETYPE_FONT__H__

#ifdef SGCT_HAS_TEXT

#include <freetype/ftglyph.h>
#include <freetype/ftstroke.h>

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace sgct::text {

/**
 * Will handle font textures and rendering. Implementation is based on
 * <a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehe's font
 * tutorial for freetype</a>.
 */
class Font {
public:
    struct FontFaceData {
        unsigned int texId = 0;
        float distToNextChar = 0.f;
        glm::vec2 pos;
        glm::vec2 size;
        FT_Glyph glyph;
    };

    /**
     * Initializes all variables needed for the font. Needs to be called before creating
     * any textures for the font.
     *
     * \param face The truetype face pointer
     * \param name FontName of the font that's being created
     * \param height Font height in pixels
     */
    Font(FT_Library lib, FT_Face face, std::string fontName, unsigned int h);

    /// Cleans up memory used by the Font and destroys the OpenGL objects
    ~Font();

    /// Get the font face data
    const Font::FontFaceData& getFontFaceData(wchar_t c);

    /// Get the vertex array id
    unsigned int getVAO() const;

    /// Get the vertex buffer objects id
    unsigned int getVBO() const;

    /// Get the display list id
    unsigned int getDisplayList() const;

    /// Get height of the font
    float getHeight() const;

    /**
     * Get the stroke (border) size.
     *
     * \return size The stroke size in pixels
     */
    long getStrokeSize() const;

    /**
     * Set the stroke (border) size.
     *
     * \param size The stroke size in pixels
     */
    void setStrokeSize(long size);

private:
    struct GlyphData {
        FT_Glyph glyph;
        FT_Glyph strokeGlyph;
        FT_Stroker stroker;
        FT_BitmapGlyph bitmapGlyph;
        FT_BitmapGlyph bitmapStrokeGlyph;
        FT_Bitmap* bitmap;
        FT_Bitmap* strokeBitmap;
    };

    void createCharacter(wchar_t c);
    bool createGlyph(wchar_t c, FontFaceData& FFDPtr);
    unsigned int generateTexture(int width, int height,
        const std::vector<unsigned char>& buffer);

    bool getPixelData(FT_Face face, int& width, int& height,
        std::vector<unsigned char>& pixels, GlyphData& gd);
    
    std::string _name;
    float _height;
    unsigned int _listId = 0;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    FT_Face	_face;
    FT_Library _library;
    FT_Fixed _strokeSize = 1;
    std::unordered_map<wchar_t, FontFaceData> _fontFaceData;
};

} // namespace sgct

#endif // SGCT_HAS_TEXT
#endif // __SGCT__FREETYPE_FONT__H__
