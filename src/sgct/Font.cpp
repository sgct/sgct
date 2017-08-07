/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/ogl_headers.h>
#include <sgct/Font.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>

/*!
Default constructor for font face data.
*/
sgct_text::FontFaceData::FontFaceData()
{
	mTexId = GL_FALSE;
	mDistToNextChar = 0.0f;
	mInterpolated = false;
}

/*!
Default constructor does not allocate any resources for the font.
The init function needs to be called before the font can actually be used
@param    fontName    Name of the font
@param    height        Height of the font
*/
sgct_text::Font::Font( const std::string & fontName, float height ) :
    mName( fontName ),
    mHeight( height )
{
	mListId = GL_FALSE;
	mVAO = GL_FALSE;
    mVBO = GL_FALSE;
}

/*!
Destructor does nothing. Fonts should be explicitly called for cleanup (Clean())
*/
sgct_text::Font::~Font()
{
    // Do nothing, need to call Clean explicitly to clean up resources
}

/*!
Initializes all variables needed for the font. Needs to be called
before creating any textures for the font
@param    face    The truetype face pointer
@param    name    FontName of the font that's being created
@aram    height    Font height in pixels
*/
void sgct_text::Font::init(FT_Library lib, FT_Face face, const std::string & name, unsigned int height )
{
	mFTLibrary = lib;
	mStrokeSize = 1;
	mFace = face;
	mName = name;
	mHeight = static_cast<float>( height );

	//setup geomerty
	if (sgct::Engine::instance()->isOGLPipelineFixed())
	{
		mListId = glGenLists(1);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating display list: %u\n", mListId);

		//So now we can create the display list
		glNewList(mListId, GL_COMPILE);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(0.0f, 1.0f, 0.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(1.0f, 0.0f, 0.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(1.0f, 1.0f, 0.0f);
		glEnd();
		glEndList();
	}
	else
	{
		glGenVertexArrays(1, &mVAO);
		glGenBuffers(1, &mVBO);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating VAO: %u\n", mVAO);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font: Generating VBO: %u\n", mVBO);

		std::vector<float> coords;
		coords.push_back(0.0f);
		coords.push_back(1.0f);
		coords.push_back(0.0f);
		coords.push_back(0.0f);

		coords.push_back(1.0f);
		coords.push_back(1.0f);
		coords.push_back(1.0f);
		coords.push_back(0.0f);

		coords.push_back(0.0f); //s
		coords.push_back(0.0f); //t
		coords.push_back(0.0f); //x
		coords.push_back(1.0f); //y

		coords.push_back(1.0f);
		coords.push_back(0.0f);
		coords.push_back(1.0f);
		coords.push_back(1.0f);

		glBindVertexArray(mVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(float), &coords[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
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
std::size_t sgct_text::Font::getNumberOfLoadedChars()
{
	return mFontFaceDataMap.size();
}

/*!
Get the stroke (border) size
@return    size    The stroke size in pixels
*/
const signed long sgct_text::Font::getStrokeSize() const
{
	return mStrokeSize;
}

/*!
Set the stroke (border) size
@param    size    The stroke size in pixels
*/
void sgct_text::Font::setStrokeSize(signed long size)
{ 
	mStrokeSize = size;
};

/*!
Cleans up memory used by the Font
*/
void sgct_text::Font::clean()
{
	if (mFontFaceDataMap.size() > 0)
	{
		if (sgct::Engine::instance()->isOGLPipelineFixed())
		{
			if (mListId != GL_FALSE)
				glDeleteLists(mListId, 1);
		}
		else
		{
			if (mVAO != GL_FALSE)
				glDeleteVertexArrays(1, &mVAO);
			if (mVBO != GL_FALSE)
				glDeleteBuffers(1, &mVBO);
		}


		//clear data
		for (auto& n : mFontFaceDataMap)
		{
			glDeleteTextures(1, &(n.second.mTexId));
			FT_Done_Glyph(n.second.mGlyph);
		}
	}
	
	mFontFaceDataMap.clear();
	FT_Done_Face(mFace);
}

sgct_text::FontFaceData * sgct_text::Font::getFontFaceData(wchar_t c)
{
	if (mFontFaceDataMap.count(c) == 0) //check if c does not exist in map
		createCharacter(c);
	
	return &mFontFaceDataMap[c];
}

void sgct_text::Font::createCharacter(wchar_t c)
{
	FontFaceData ffd;
	
	//create glyph
	if (createGlyph(c, &ffd))
	{
		mFontFaceDataMap[c] = ffd;
	}
}

bool sgct_text::Font::createGlyph(wchar_t c, FontFaceData * FFDPtr)
{
	//Load the Glyph for our character.
	/*
	Hints:
	http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX
	*/

	FT_UInt char_index = FT_Get_Char_Index(mFace, static_cast<FT_ULong>(c));
	if (char_index == 0)
	{
		std::string mName;                // Holds the font name
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Font %s: Missing face for char %u!\n", mName.c_str(), static_cast<unsigned int>(c));
	}

	if (FT_Load_Glyph(mFace, char_index, FT_LOAD_FORCE_AUTOHINT))
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Font %s: FT_Load_Glyph failed for char %u!\n", mName.c_str(), static_cast<unsigned int>(c));
		return false;
	}

	int width;
	int height;
	unsigned char * pixels = NULL;

	//load pixel data
	GlyphData gd;
	if (!getPixelData(mFace, width, height, &pixels, &gd))
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Font %s: FT_Get_Glyph failed for char %u.\n", mName.c_str(), static_cast<unsigned int>(c));
		return false;
	}

	//create texture
	if (char_index > 0) //skip null
		FFDPtr->mTexId = generateTexture(width, height, pixels);
	else
		FFDPtr->mTexId = GL_FALSE;

	//With the texture created, we don't need to expanded data anymore
	delete[] pixels;

	//setup geometry data
	FFDPtr->mPos.x = static_cast<float>(gd.mBitmapGlyph->left);
	FFDPtr->mPos.y = static_cast<float>(gd.mBitmapGlyph->top - gd.mBitmapPtr->rows);
	FFDPtr->mSize.x = static_cast<float>(width);
	FFDPtr->mSize.y = static_cast<float>(height);

	//delete the stroke glyph
	FT_Stroker_Done(gd.mStroker);
	FT_Done_Glyph(gd.mStrokeGlyph);
	
	// Can't delete them while they are used, delete when font is cleaned
	FFDPtr->mGlyph = gd.mGlyph;
	FFDPtr->mDistToNextChar = static_cast<float>(mFace->glyph->advance.x >> 6);

	return true;
}

unsigned int sgct_text::Font::generateTexture(int width, int height, unsigned char * data)
{
	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	/*
	SGCT2 change: Use non-power-of-two textures for better quality
	*/
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (sgct::Engine::instance()->isOGLPipelineFixed())
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_LUMINANCE_ALPHA, width, height,
			0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RG, width, height,
			0, GL_RG, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	return tex;
}

bool sgct_text::Font::getPixelData(FT_Face face, int & width, int & height, unsigned char ** pixels, GlyphData * gd)
{
	//Move the face's glyph into a Glyph object.
	if (FT_Get_Glyph(face->glyph, &(gd->mGlyph)) || FT_Get_Glyph(face->glyph, &(gd->mStrokeGlyph)))
	{
		return false;
	}

	gd->mStroker = NULL;
	FT_Error error = FT_Stroker_New(mFTLibrary, &(gd->mStroker));
	if (!error)
	{
		FT_Stroker_Set(gd->mStroker, 64 * mStrokeSize,
			FT_STROKER_LINECAP_ROUND,
			FT_STROKER_LINEJOIN_ROUND,
			0);

		error = FT_Glyph_Stroke(&(gd->mStrokeGlyph), gd->mStroker, 1);
	}

	//Convert the glyph to a bitmap.
	FT_Glyph_To_Bitmap(&(gd->mGlyph), ft_render_mode_normal, 0, 1);
	gd->mBitmapGlyph = (FT_BitmapGlyph)(gd->mGlyph);

	FT_Glyph_To_Bitmap(&(gd->mStrokeGlyph), ft_render_mode_normal, 0, 1);
	gd->mBitmapStrokeGlyph = (FT_BitmapGlyph)(gd->mStrokeGlyph);

	//This pointer will make accessing the bitmap easier
	gd->mBitmapPtr = &(gd->mBitmapGlyph->bitmap);
	gd->mStrokeBitmapPtr = &(gd->mBitmapStrokeGlyph->bitmap);

	//Use our helper function to get the widths of
	//the bitmap data that we will need in order to create
	//our texture.
	width = gd->mStrokeBitmapPtr->width; //stroke is always larger
	height = gd->mStrokeBitmapPtr->rows;

	//Allocate memory for the texture data.
	(*pixels) = new unsigned char[2 * width * height];

	//read alpha to one channel and stroke - alpha in the second channel
	//We use the ?: operator so that value which we use
	//will be 0 if we are in the padding zone, and whatever
	//is the the Freetype bitmap otherwise.
	int k, l;
	int diff_offset[2];
	diff_offset[0] = (gd->mStrokeBitmapPtr->width - gd->mBitmapPtr->width) >> 1;
	diff_offset[1] = (gd->mStrokeBitmapPtr->rows - gd->mBitmapPtr->rows) >> 1;
	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			k = i - diff_offset[0];
			l = j - diff_offset[1];
			(*pixels)[2 * (i + j*width)] =
				(k >= gd->mBitmapPtr->width || l >= gd->mBitmapPtr->rows || k < 0 || l < 0) ?
				0 : gd->mBitmapPtr->buffer[k + gd->mBitmapPtr->width*l];

			unsigned char strokeVal = (i >= gd->mStrokeBitmapPtr->width || j >= gd->mStrokeBitmapPtr->rows) ?
				0 : gd->mStrokeBitmapPtr->buffer[i + gd->mStrokeBitmapPtr->width*j];

			//simple union
			(*pixels)[2 * (i + j*width) + 1] = strokeVal < (*pixels)[2 * (i + j*width)] ?
				(*pixels)[2 * (i + j*width)] : strokeVal;
		}
	}

	return true;
}