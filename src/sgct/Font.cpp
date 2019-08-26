/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/Font.h>

#include <sgct/ogl_headers.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <array>

namespace sgct_text {

bool Font::operator<(const Font & rhs) const {
    return mName.compare(rhs.mName) < 0 || (mName == rhs.mName && mHeight < rhs.mHeight);
}

/*! Equal to Font comparison operator */
bool Font::operator==(const Font & rhs) const {
    return mName == rhs.mName && mHeight == rhs.mHeight;
}

/*!
Default constructor does not allocate any resources for the font.
The init function needs to be called before the font can actually be used
@param    fontName    Name of the font
@param    height        Height of the font
*/
Font::Font(std::string fontName, float height)
    : mName(std::move(fontName))
    , mHeight(height)
{}

/*!
Initializes all variables needed for the font. Needs to be called
before creating any textures for the font
@param    face    The truetype face pointer
@param    name    FontName of the font that's being created
@aram    height    Font height in pixels
*/
void Font::init(FT_Library lib, FT_Face face, std::string name, unsigned int height) {
    mFTLibrary = lib;
    mStrokeSize = 1;
    mFace = face;
    mName = std::move(name);
    mHeight = static_cast<float>(height);

    //setup geomerty
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        mListId = glGenLists(1);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font: Generating display list: %u\n",
            mListId
        );

        //So now we can create the display list
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
            "Font: Generating VAO: %u\n",
            mVAO
        );
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font: Generating VBO: %u\n",
            mVBO
        );

        std::array<float, 16> coords = {
            0.f, 1.f, 0.f, 0.f,
            1.f, 1.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
            1.f, 0.f, 1.f, 1.f
        };

        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            coords.size() * sizeof(float),
            coords.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, 
                                // but must match the layout in the shader.
            2,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            4 * sizeof(float),    // stride
            reinterpret_cast<void*>(0) // array buffer offset
            );

        glVertexAttribPointer(
            1,                  // attribute 1
            2,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            4 * sizeof(float),    // stride
            reinterpret_cast<void*>(8) // array buffer offset
            );

        //unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

/*!
Counts the number of textures used by this font.
*/
size_t Font::getNumberOfLoadedChars() {
    return mFontFaceDataMap.size();
}

/*!
Get the stroke (border) size
@return    size    The stroke size in pixels
*/
signed long Font::getStrokeSize() const {
    return mStrokeSize;
}

/*!
Set the stroke (border) size
@param    size    The stroke size in pixels
*/
void Font::setStrokeSize(signed long size) {
    mStrokeSize = size;
};

/*!
Cleans up memory used by the Font
*/
void Font::clean() {
    if (!mFontFaceDataMap.empty()) {
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            if (mListId != GL_FALSE) {
                glDeleteLists(mListId, 1);
            }
        }
        else {
            if (mVAO != GL_FALSE) {
                glDeleteVertexArrays(1, &mVAO);
            }
            if (mVBO != GL_FALSE) {
                glDeleteBuffers(1, &mVBO);
            }
        }


        // clear data
        for (const std::pair<const wchar_t, FontFaceData>& n : mFontFaceDataMap) {
            glDeleteTextures(1, &(n.second.mTexId));
            FT_Done_Glyph(n.second.mGlyph);
        }
    }
    
    mFontFaceDataMap.clear();
    FT_Done_Face(mFace);
}

const Font::FontFaceData& Font::getFontFaceData(wchar_t c) {
    if (mFontFaceDataMap.count(c) == 0) {
        //check if c does not exist in map
        createCharacter(c);
    }
    
    return mFontFaceDataMap[c];
}

/*! Get the vertex array id */
unsigned int Font::getVAO() const {
    return mVAO;
}

/*! Get the vertex buffer objects id */
unsigned int Font::getVBO() const {
    return mVBO;
}

/*! Get the display list id */
unsigned int Font::getDisplayList() const {
    return mListId;
}

/*! Get height of the font */
float Font::getHeight() const {
    return mHeight;
}

void Font::createCharacter(wchar_t c) {
    FontFaceData ffd;
    
    // create glyph
    if (createGlyph(c, ffd)) {
        mFontFaceDataMap[c] = ffd;
    }
}

bool Font::createGlyph(wchar_t c, FontFaceData& ffd) {
    //Load the Glyph for our character.
    /*
    Hints:
    http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX
    */

    FT_UInt char_index = FT_Get_Char_Index(mFace, static_cast<FT_ULong>(c));
    if (char_index == 0) {
        std::string mName;                // Holds the font name
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Font %s: Missing face for char %u!\n",
            mName.c_str(), static_cast<unsigned int>(c)
        );
    }

    if (FT_Load_Glyph(mFace, char_index, FT_LOAD_FORCE_AUTOHINT)) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Font %s: FT_Load_Glyph failed for char %u!\n",
            mName.c_str(), static_cast<unsigned int>(c)
        );
        return false;
    }

    int width;
    int height;
    unsigned char* pixels = nullptr;

    //load pixel data
    GlyphData gd;
    if (!getPixelData(mFace, width, height, &pixels, gd)) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Font %s: FT_Get_Glyph failed for char %u.\n",
            mName.c_str(), static_cast<unsigned int>(c)
        );
        return false;
    }

    //create texture
    if (char_index > 0) {
        //skip null
        ffd.mTexId = generateTexture(width, height, pixels);
    }
    else {
        ffd.mTexId = GL_FALSE;
    }

    //With the texture created, we don't need to expanded data anymore
    delete[] pixels;

    //setup geometry data
    ffd.mPos.x = static_cast<float>(gd.mBitmapGlyph->left);
    ffd.mPos.y = static_cast<float>(gd.mBitmapGlyph->top - gd.mBitmapPtr->rows);
    ffd.mSize.x = static_cast<float>(width);
    ffd.mSize.y = static_cast<float>(height);

    //delete the stroke glyph
    FT_Stroker_Done(gd.mStroker);
    FT_Done_Glyph(gd.mStrokeGlyph);
    
    // Can't delete them while they are used, delete when font is cleaned
    ffd.mGlyph = gd.mGlyph;
    ffd.mDistToNextChar = static_cast<float>(mFace->glyph->advance.x >> 6);

    return true;
}

unsigned int Font::generateTexture(int width, int height, unsigned char* data) {
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    /*
    SGCT2 change: Use non-power-of-two textures for better quality
    */
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
            data
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
            data
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

bool Font::getPixelData(FT_Face face, int& width, int& height, unsigned char** pixels,
                        GlyphData& gd)
{
    //Move the face's glyph into a Glyph object.
    if (FT_Get_Glyph(face->glyph, &(gd.mGlyph)) ||
        FT_Get_Glyph(face->glyph, &(gd.mStrokeGlyph)))
    {
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

    //Convert the glyph to a bitmap.
    FT_Glyph_To_Bitmap(&(gd.mGlyph), ft_render_mode_normal, 0, 1);
    gd.mBitmapGlyph = (FT_BitmapGlyph)(gd.mGlyph);

    FT_Glyph_To_Bitmap(&(gd.mStrokeGlyph), ft_render_mode_normal, 0, 1);
    gd.mBitmapStrokeGlyph = (FT_BitmapGlyph)(gd.mStrokeGlyph);

    //This pointer will make accessing the bitmap easier
    gd.mBitmapPtr = &(gd.mBitmapGlyph->bitmap);
    gd.mStrokeBitmapPtr = &(gd.mBitmapStrokeGlyph->bitmap);

    //Use our helper function to get the widths of
    //the bitmap data that we will need in order to create
    //our texture.
    width = gd.mStrokeBitmapPtr->width; //stroke is always larger
    height = gd.mStrokeBitmapPtr->rows;

    //Allocate memory for the texture data.
    (*pixels) = new unsigned char[2 * width * height];

    //read alpha to one channel and stroke - alpha in the second channel
    //We use the ?: operator so that value which we use
    //will be 0 if we are in the padding zone, and whatever
    //is the the Freetype bitmap otherwise.
    int k, l;
    int diff_offset[2];
    diff_offset[0] = (gd.mStrokeBitmapPtr->width - gd.mBitmapPtr->width) >> 1;
    diff_offset[1] = (gd.mStrokeBitmapPtr->rows - gd.mBitmapPtr->rows) >> 1;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            k = i - diff_offset[0];
            l = j - diff_offset[1];
            (*pixels)[2 * (i + j * width)] =
                (k >= gd.mBitmapPtr->width || l >= gd.mBitmapPtr->rows || k < 0 || l < 0) ?
                0 :
                gd.mBitmapPtr->buffer[k + gd.mBitmapPtr->width * l];

            unsigned char strokeVal =
                (i >= gd.mStrokeBitmapPtr->width || j >= gd.mStrokeBitmapPtr->rows) ?
                0 :
                gd.mStrokeBitmapPtr->buffer[i + gd.mStrokeBitmapPtr->width * j];

            //simple union
            (*pixels)[2 * (i + j*width) + 1] = strokeVal < (*pixels)[2 * (i + j*width)] ?
                (*pixels)[2 * (i + j*width)] :
                strokeVal;
        }
    }

    return true;
}

} // namespace sgct_text
