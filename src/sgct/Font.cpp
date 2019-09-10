/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/Font.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/ogl_headers.h>
#include <array>

namespace sgct_text {

Font::Font(FT_Library lib, FT_Face face, std::string name, unsigned int height)
    : mFTLibrary(lib)
    , mFace(face)
    , mName(std::move(name))
    , mHeight(static_cast<float>(height))
{
    // setup geometry
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        mListId = glGenLists(1);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font: Generating display list: %u\n", mListId
        );

        // So now we can create the display list
        glNewList(mListId, GL_COMPILE);

        glBegin(GL_QUADS);
        glTexCoord2f(0.f, 0.f);
        glVertex3f(0.f, 1.f, 0.f);

        glTexCoord2f(0.f, 1.f);
        glVertex3f(0.f, 0.f, 0.f);

        glTexCoord2f(1.f, 1.f);
        glVertex3f(1.f, 0.f, 0.f);

        glTexCoord2f(1.f, 0.f);
        glVertex3f(1.f, 1.f, 0.f);
        glEnd();
        glEndList();
    }
    else {
        glGenVertexArrays(1, &mVAO);
        glGenBuffers(1, &mVBO);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font: Generating VAO: %u\n", mVAO
        );
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font: Generating VBO: %u\n", mVBO
        );

        std::array<float, 16> c = {
            0.f, 1.f, 0.f, 0.f,
            1.f, 1.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
            1.f, 0.f, 1.f, 1.f
        };

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
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
}

Font::~Font() {
    if (!mFontFaceDataMap.empty()) {
        glDeleteLists(mListId, 1);
        glDeleteVertexArrays(1, &mVAO);
        glDeleteBuffers(1, &mVBO);

        for (const std::pair<const wchar_t, FontFaceData>& n : mFontFaceDataMap) {
            glDeleteTextures(1, &(n.second.mTexId));
            FT_Done_Glyph(n.second.mGlyph);
        }
    }

    mFontFaceDataMap.clear();
    FT_Done_Face(mFace);
}

long Font::getStrokeSize() const {
    return mStrokeSize;
}

void Font::setStrokeSize(long size) {
    mStrokeSize = size;
};

const Font::FontFaceData& Font::getFontFaceData(wchar_t c) {
    if (mFontFaceDataMap.count(c) == 0) {
        // check if c does not exist in map
        createCharacter(c);
    }
    
    return mFontFaceDataMap[c];
}

unsigned int Font::getVAO() const {
    return mVAO;
}

unsigned int Font::getVBO() const {
    return mVBO;
}

unsigned int Font::getDisplayList() const {
    return mListId;
}

float Font::getHeight() const {
    return mHeight;
}

void Font::createCharacter(wchar_t c) {
    FontFaceData ffd;
    const bool success = createGlyph(c, ffd);
    if (success) {
        mFontFaceDataMap[c] = std::move(ffd);
    }
}

bool Font::createGlyph(wchar_t c, FontFaceData& ffd) {
    // Load the Glyph for our character.
    // Hints:
    // www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX

    FT_UInt charIndex = FT_Get_Char_Index(mFace, static_cast<FT_ULong>(c));
    if (charIndex == 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font %s: Missing face for char %u!\n",
            mName.c_str(), static_cast<unsigned int>(c)
        );
    }

    FT_Error loadError = FT_Load_Glyph(mFace, charIndex, FT_LOAD_FORCE_AUTOHINT);
    if (loadError) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Font %s: FT_Load_Glyph failed for char %u!\n",
            mName.c_str(), static_cast<unsigned int>(c)
        );
        return false;
    }

    // load pixel data
    int width;
    int height;
    std::vector<unsigned char> pixels;
    GlyphData gd;
    const bool success = getPixelData(mFace, width, height, pixels, gd);
    if (!success) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Font %s: FT_Get_Glyph failed for char %u.\n",
            mName.c_str(), static_cast<unsigned int>(c)
        );
        return false;
    }

    // create texture
    if (charIndex > 0) {
        ffd.mTexId = generateTexture(width, height, pixels);
    }
    else {
        ffd.mTexId = 0;
    }

    // With the texture created, we don't need to expanded data anymore
    pixels.clear();

    // setup geometry data
    ffd.mPos.x = static_cast<float>(gd.mBitmapGlyph->left);
    ffd.mPos.y = static_cast<float>(gd.mBitmapGlyph->top - gd.mBitmapPtr->rows);
    ffd.mSize.x = static_cast<float>(width);
    ffd.mSize.y = static_cast<float>(height);

    // delete the stroke glyph
    FT_Stroker_Done(gd.mStroker);
    FT_Done_Glyph(gd.mStrokeGlyph);
    
    // Can't delete them while they are used, delete when font is cleaned
    ffd.mGlyph = gd.mGlyph;
    ffd.mDistToNextChar = static_cast<float>(mFace->glyph->advance.x / 64);

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

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_COMPRESSED_LUMINANCE_ALPHA,
            width,
            height,
            0,
            GL_LUMINANCE_ALPHA,
            GL_UNSIGNED_BYTE,
            buffer.data()
        );
    }
    else {
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
    }

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
    FT_Error glyphErr = FT_Get_Glyph(face->glyph, &(gd.mGlyph));
    FT_Error strokeErr = FT_Get_Glyph(face->glyph, &(gd.mStrokeGlyph));
    if (glyphErr || strokeErr) {
        return false;
    }

    gd.mStroker = nullptr;
    FT_Error error = FT_Stroker_New(mFTLibrary, &(gd.mStroker));
    if (!error) {
        FT_Stroker_Set(
            gd.mStroker,
            64 * mStrokeSize,
            FT_STROKER_LINECAP_ROUND,
            FT_STROKER_LINEJOIN_ROUND,
            0
        );

        error = FT_Glyph_Stroke(&(gd.mStrokeGlyph), gd.mStroker, 1);
    }

    // Convert the glyph to a bitmap
    FT_Glyph_To_Bitmap(&(gd.mGlyph), ft_render_mode_normal, 0, 1);
    gd.mBitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(gd.mGlyph);

    FT_Glyph_To_Bitmap(&(gd.mStrokeGlyph), ft_render_mode_normal, 0, 1);
    gd.mBitmapStrokeGlyph = reinterpret_cast<FT_BitmapGlyph>(gd.mStrokeGlyph);

    // This pointer will make accessing the bitmap easier
    gd.mBitmapPtr = &(gd.mBitmapGlyph->bitmap);
    gd.mStrokeBitmapPtr = &(gd.mBitmapStrokeGlyph->bitmap);

    // Use our helper function to get the widths of the bitmap data that we will need in
    // order to create our texture
    width = gd.mStrokeBitmapPtr->width;
    height = gd.mStrokeBitmapPtr->rows;

    // Allocate memory for the texture data
    pixels.resize(2 * width * height);
    std::fill(pixels.begin(), pixels.end(), unsigned char(0));

    // read alpha to one channel and stroke - alpha in the second channel. We use the ?:
    // operator so that value which we use will be 0 if we are in the padding zone, and
    // whatever is the the Freetype bitmap otherwise
    const int offsetWidth = (gd.mStrokeBitmapPtr->width - gd.mBitmapPtr->width) / 2;
    const int offsetRows = (gd.mStrokeBitmapPtr->rows - gd.mBitmapPtr->rows) / 2;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const int k = i - offsetWidth;
            const int l = j - offsetRows;

            const int idx = 2 * (i + j * width);
            if (k >= gd.mBitmapPtr->width || l >= gd.mBitmapPtr->rows || k < 0 || l < 0) {
                pixels[idx] = 0;
            }
            else {
                pixels[idx] = gd.mBitmapPtr->buffer[k + gd.mBitmapPtr->width * l];
            }

            const bool strokeInRange = i >= gd.mStrokeBitmapPtr->width ||
                                       j >= gd.mStrokeBitmapPtr->rows;
            unsigned char strokeVal = strokeInRange ?
                0 : gd.mStrokeBitmapPtr->buffer[i + gd.mStrokeBitmapPtr->width * j];

            pixels[idx + 1] = strokeVal < pixels[idx] ? pixels[idx] : strokeVal;
        }
    }

    return true;
}

} // namespace sgct_text
