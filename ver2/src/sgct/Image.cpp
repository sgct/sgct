/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <stdio.h>
#include <fstream>
#include "../include/external/png.h"
#include <stdlib.h>

#ifdef __WIN32__
#include "../include/external/pngpriv.h"
#endif

#include "../include/sgct/Image.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTSettings.h"

sgct_core::Image::Image()
{
	mFilename = NULL;
	mData = NULL;
	mRowPtrs = NULL;
}

sgct_core::Image::~Image()
{
	cleanup();
}

bool sgct_core::Image::load(const char * filename)
{
	int length = 0;

	while(filename[length] != '\0')
		length++;

	char type[5];
	if(length > 4)
	{
		type[0] = filename[length-4];
		type[1] = filename[length-3];
		type[2] = filename[length-2];
		type[3] = filename[length-1];
		type[4] = '\0';

		if( strcmp(".PNG", type) == 0 || strcmp(".png", type) == 0 )
			return loadPNG(filename);
		else
		{
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: Unknown filesuffix: \"%s\"\n", type);
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: Loading failed (bad filename: %s)\n", filename);
		return false;
	}
}

#define PNG_BYTES_TO_CHECK 8

bool sgct_core::Image::loadPNG(const char *filename)
{
	mFilename = NULL;
	if( filename == NULL || strlen(filename) < 5) //one char + dot and suffix and is 5 char
	{
	    return false;
	}

	//copy filename
	mFilename = new char[strlen(filename)+1];
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( strcpy_s(mFilename, strlen(filename)+1, filename ) != 0)
		return false;
    #else
		strcpy(mFilename, filename );
    #endif

	unsigned char *pb;
	png_structp png_ptr;
	png_infop info_ptr;
	char header[PNG_BYTES_TO_CHECK];
	//int numChannels;
	int r, color_type, bpp;

	FILE *fp = NULL;
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename, "rb") != 0 && !fp )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't open PNG texture file '%s'\n", mFilename);
		return false;
	}
    #else
    fp = fopen(mFilename, "rb");
    if( fp == NULL )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't open PNG texture file '%s'\n", mFilename);
		return false;
	}
    #endif

	size_t result = fread( header, 1, PNG_BYTES_TO_CHECK, fp );
	if( result != PNG_BYTES_TO_CHECK || png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Texture file '%s' is not in PNG format\n", mFilename);
		fclose(fp);
		return false;
	}

	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( png_ptr == NULL )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't initialize PNG file for reading: %s\n", mFilename);
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr == NULL )
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't allocate memory to read PNG file: %s\n", mFilename);
		return false;
	}

	if( setjmp(png_jmpbuf(png_ptr)) )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Exception occurred while reading PNG file: %s\n", mFilename);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	png_read_png( png_ptr, info_ptr,
                PNG_TRANSFORM_STRIP_16 |
                PNG_TRANSFORM_PACKING |
                PNG_TRANSFORM_EXPAND, NULL );

	//channels = png_get_channels(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&mSize_x, (png_uint_32 *)&mSize_y, &bpp, &color_type, NULL, NULL, NULL);

	if(color_type == PNG_COLOR_TYPE_GRAY )
	{
		mChannels = 1;
		if(bpp < 8)
		{
			png_set_expand_gray_1_2_4_to_8(png_ptr);
			png_read_update_info(png_ptr, info_ptr);
		}
	}
	else if(color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		mChannels = 2;
	else if(color_type == PNG_COLOR_TYPE_RGB)
		mChannels = 3;
	else if(color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		mChannels = 4;
	else
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Unsupported format '%s'\n", mFilename );
		fclose(fp);
		return false;
	}

	mData = pb = new unsigned char[ mChannels * mSize_x * mSize_y ];
	png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
	for( r = (int)png_get_image_height(png_ptr, info_ptr) - 1 ; r >= 0 ; r-- )
	{
		//png_bytep row = info_ptr->row_pointers[r];
        png_bytep row = row_pointers[r];
		int rowbytes = static_cast<int>(png_get_rowbytes(png_ptr, info_ptr));
		int c;
		for( c = 0 ; c < rowbytes ; c++ )
			*(pb)++ = row[c];
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	fclose(fp);

	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image loaded %s\n", mFilename);

	//clean up filename
	if( mFilename != NULL )
	{
		delete [] mFilename;
		mFilename = NULL;
	}

	return true;
}

/*!
	Save the buffer to file. Type is automatically set by filename suffix.
*/
bool sgct_core::Image::save()
{
	int length = 0;

	if(mFilename == NULL)
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Filename not set for saving image.\n");
		return false;
	}

	while(mFilename[length] != '\0')
		length++;

	char type[5];
	if(length > 5)
	{
		type[0] = mFilename[length-4];
		type[1] = mFilename[length-3];
		type[2] = mFilename[length-2];
		type[3] = mFilename[length-1];
		type[4] = '\0';

		if( strcmp(".PNG", type) == 0 || strcmp(".png", type) == 0 )
			return savePNG( sgct::SGCTSettings::Instance()->getPNGCompressionLevel() );
		if( strcmp(".TGA", type) == 0 || strcmp(".tga", type) == 0 )
			return saveTGA();
		else
		{
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to save image! Unknown filesuffix: \"%s\"\n", type);
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to save image! Bad filename: %s\n", mFilename);
		return false;
	}
}

/*!
		Compression levels 1-9.
		-1 = Default compression
		0 = No compression
		1 = Best speed
		9 = Best compression
*/
bool sgct_core::Image::savePNG(const char * filename, int compressionLevel)
{
	setFilename( filename );
	return savePNG( compressionLevel );
}

bool sgct_core::Image::savePNG(int compressionLevel)
{
	if( mData == NULL && !allocateOrResizeData())
		return false;

	FILE *fp = NULL;
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename, "wb") != 0 && !fp )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't create PNG texture file '%s'\n", mFilename);
		return false;
	}
    #else
    fp = fopen(mFilename, "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't create PNG texture file '%s'\n", mFilename);
		return false;
	}
    #endif

	/* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
		return false;

	//set compression
	png_set_compression_level( png_ptr, compressionLevel );
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE );

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
		return false;

    if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	png_init_io(png_ptr, fp);

	int color_type = -1;
	switch( mChannels )
	{
	case 1:
		color_type = PNG_COLOR_TYPE_GRAY;
		break;

	case 2:
		color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;

	case 3:
		color_type = PNG_COLOR_TYPE_RGB;
		break;

	case 4:
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		break;
	}

	if( color_type == -1 )
		return false;

	/* write header */
    png_set_IHDR(png_ptr, info_ptr, mSize_x, mSize_y,
        8, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

	/* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	for (int y = (mSize_y-1);  y >= 0;  y--)
        mRowPtrs[(mSize_y-1)-y] = (png_bytep) &mData[y * mSize_x * mChannels];
    png_write_image(png_ptr, mRowPtrs);

	/* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
		return false;

    png_write_end(png_ptr, NULL);

	png_destroy_write_struct (&png_ptr, &info_ptr);

	fclose(fp);

	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image '%s' was saved successfully!\n", mFilename);

	return true;
}

bool sgct_core::Image::saveTGA()
{
	if( mData == NULL && !allocateOrResizeData())
		return false;

	FILE *fp = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename, "wb") != 0 && !fp )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't create TGA texture file '%s'\n", mFilename);
		return false;
	}
#else
    fp = fopen(mFilename, "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't create TGA texture file '%s'\n", mFilename);
		return false;
	}
#endif

	if( mChannels == 2 )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Can't create TGA texture file '%s'.\nLuminance alpha not supported by the TGA format.\n", mFilename);
		return false;
	}

	/*
	TGA data type field

	0  -  No image data included.
    1  -  Uncompressed, color-mapped images.
    2  -  Uncompressed, RGB images.
    3  -  Uncompressed, black and white images.
    9  -  Runlength encoded color-mapped images.
	10  -  Runlength encoded RGB images.
	11  -  Compressed, black and white images.
	32  -  Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
	33  -  Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process.
	*/

	unsigned char data_type;
	switch( mChannels )
	{
	default:
		data_type = 2;
		break;

	case 1:
		data_type = 3; //bw
		break;
	}

	// The image header
	unsigned char header[ 18 ] = { 0 };
	header[  2 ] = data_type; //datatype
	header[ 12 ] =  mSize_x        & 0xFF;
	header[ 13 ] = (mSize_x  >> 8) & 0xFF;
	header[ 14 ] =  mSize_y       & 0xFF;
	header[ 15 ] = (mSize_y >> 8) & 0xFF;
	header[ 16 ] = mChannels * 8;  // bits per pixel

	fwrite(header, sizeof(unsigned char), sizeof(header), fp);

	// The file footer. This part is totally optional.
	static const char footer[ 26 ] =
		"\0\0\0\0"  // no extension area
		"\0\0\0\0"  // no developer directory
		"TRUEVISION-XFILE"  // yep, this is a TGA file
		".";

	for(int y=0; y<mSize_y; y++)
		fwrite( &mData[y * mSize_x * mChannels], mChannels, mSize_x, fp);

	fwrite(footer, sizeof(char), sizeof(footer), fp);

	fclose(fp);

	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image '%s' was saved successfully!\n", mFilename);

	return true;
}

void sgct_core::Image::setFilename(const char * filename)
{
	if( mFilename != NULL )
	{
		delete [] mFilename;
		mFilename = NULL;
	}

	if( filename == NULL || strlen(filename) < 5) //one char + dot and suffix and is 5 char
	{
	    sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Invalid filename!\n");
		return;
	}

	//copy filename
	mFilename = new char[strlen(filename)+1];
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( strcpy_s(mFilename, strlen(filename)+1, filename ) != 0)
		return;
    #else
		strcpy(mFilename, filename );
    #endif
}

void sgct_core::Image::cleanup()
{
	if(mData != NULL)
	{
		delete [] mData;
		mData = NULL;
	}

	if(mRowPtrs != NULL)
	{
		delete [] mRowPtrs;
		mRowPtrs = NULL;
	}

	if( mFilename != NULL )
	{
		delete [] mFilename;
		mFilename = NULL;
	}
}

unsigned char * sgct_core::Image::getData()
{
	return mData;
}
int sgct_core::Image::getChannels()
{
	return mChannels;
}

int sgct_core::Image::getSizeX()
{
	return mSize_x;
}

int sgct_core::Image::getSizeY()
{
	return mSize_y;
}

void sgct_core::Image::setDataPtr(unsigned char * dPtr)
{
	mData = dPtr;
}

void sgct_core::Image::setSize(int width, int height)
{
	mSize_x = width;
	mSize_y = height;
}

void sgct_core::Image::setChannels(int channels)
{
	mChannels = channels;
}

bool sgct_core::Image::allocateOrResizeData()
{
	if(mSize_x <= 0 || mSize_y <= 0 || mChannels <= 0)
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Invalid image size %dx%d %d channels!\n",
			mSize_x, mSize_y, mChannels);
		return false;
	}

	if(mData != NULL) //re-allocate
	{
		delete [] mData;
		mData = NULL;

		delete [] mRowPtrs;
		mRowPtrs = NULL;
	}

	try
	{
		mData = new unsigned char[ mChannels * mSize_x * mSize_y ];
	}
	catch(std::bad_alloc& ba)
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Failed to allocate %d bytes of image data (%s).\n", mChannels * mSize_x * mSize_y, ba.what());
		mData = NULL;
		return false;
	}

	try
	{
		mRowPtrs = new png_bytep[ mSize_y ];
	}
	catch(std::bad_alloc& ba)
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Failed to allocate pointers for image data (%s).\n", ba.what());
		mRowPtrs = NULL;
		return false;
	}

	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Info: Allocated %d bytes for image data\n", mChannels * mSize_x * mSize_y);
	return true;
}
