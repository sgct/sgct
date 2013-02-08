/*************************************************************************
Copyright (c) 2012 Miroslav Andel
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

sgct_core::Image::Image()
{
	mFilename = NULL;
	mData = NULL;
}

bool sgct_core::Image::load(const char * filename)
{
	int length = 0;

	while(filename[length] != '\0')
		length++;

	char type[5];
	if(length > 5)
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
			sgct::MessageHandler::Instance()->print("Unknown filesuffix: \"%s\"\n", type);
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::Instance()->print("Image loading failed (bad filename: %s)\n", filename);
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
		sgct::MessageHandler::Instance()->print("Can't open PNG texture file '%s'\n", mFilename);
		return false;
	}
    #else
    fp = fopen(mFilename, "rb");
    if( fp == NULL )
	{
		sgct::MessageHandler::Instance()->print("Can't open PNG texture file '%s'\n", mFilename);
		return false;
	}
    #endif

	size_t result = fread( header, 1, PNG_BYTES_TO_CHECK, fp );
	if( result != PNG_BYTES_TO_CHECK || png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) )
	{
		sgct::MessageHandler::Instance()->print("Texture file '%s' is not in PNG format\n", mFilename);
		fclose(fp);
		return false;
	}

	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( png_ptr == NULL )
	{
		sgct::MessageHandler::Instance()->print("Can't initialize PNG file for reading: %s\n", mFilename);
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr == NULL )
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		sgct::MessageHandler::Instance()->print("Can't allocate memory to read PNG file: %s\n", mFilename);
		return false;
	}

	if( setjmp(png_jmpbuf(png_ptr)) )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		sgct::MessageHandler::Instance()->print("Exception occurred while reading PNG file: %s\n", mFilename);
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
		sgct::MessageHandler::Instance()->print("Unsupported format '%s'\n", mFilename );
		fclose(fp);
		return false;
	}

	mData = pb = new unsigned char[ mChannels * mSize_x * mSize_y ];
	//mData = pb = (unsigned char*)malloc( sizeof(unsigned char)*( mChannels * mSize_x * mSize_y ) );
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

	sgct::MessageHandler::Instance()->print("Image loaded %s\n", mFilename);
	return true;
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
	if( mData == NULL )
	{
		sgct::MessageHandler::Instance()->print("Can't save PNG texture file '%s'. Out of memory!\n", mFilename);
		return false;
	}
	
	FILE *fp = NULL;
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename, "wb") != 0 && !fp )
	{
		sgct::MessageHandler::Instance()->print("Can't create PNG texture file '%s'\n", mFilename);
		return false;
	}
    #else
    fp = fopen(mFilename, "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::Instance()->print("Can't create PNG texture file '%s'\n", mFilename);
		return false;
	}
    #endif

	/* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
		return false;

	//set compression
	png_set_compression_level( png_ptr, compressionLevel );

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

	//png_bytep * rows = (png_bytep*) malloc(mSize_y * sizeof(png_bytep));
	png_bytep * rows = NULL;
	rows = new png_bytep[mSize_y];
    if (rows == NULL)//alloc error
		return false;

	for (int y = (mSize_y-1);  y >= 0;  y--)
        rows[(mSize_y-1)-y] = (png_bytep) &mData[y * mSize_x * mChannels];
    png_write_image(png_ptr, rows);
	
	//free(rows); //clean up
	delete [] rows;

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
		return false;

    png_write_end(png_ptr, NULL);

	fclose(fp);

	sgct::MessageHandler::Instance()->print("Image '%s' was saved successfully!\n", mFilename);
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
	    sgct::MessageHandler::Instance()->print("Image error: Invalid filename!\n");
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

void sgct_core::Image::cleanup(bool releaseMemory)
{
	if(releaseMemory && mData != NULL)
	{
		delete [] mData;
		mData = NULL;
		sgct::MessageHandler::Instance()->print("Image data deleted %s\n", mFilename);
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

void sgct_core::Image::allocateOrResizeData()
{
	if(mData == NULL)
	{
		try
		{
			mData = new unsigned char[ mChannels * mSize_x * mSize_y ];
		}
		catch(std::bad_alloc& ba)
		{
			sgct::MessageHandler::Instance()->print("Error: Failed to allocate %d bytes of image data (%s)\n", mChannels * mSize_x * mSize_y, ba.what());
			mData = NULL;
			return;
		}
	}
	else //re-allocate
	{
		delete [] mData;
		mData = NULL;

		try
		{
			mData = new unsigned char[ mChannels * mSize_x * mSize_y ];
		}
		catch(std::bad_alloc& ba)
		{
			sgct::MessageHandler::Instance()->print("Error: Failed to allocate %d bytes of image data (%s)\n", mChannels * mSize_x * mSize_y, ba.what());
			mData = NULL;
			return;
		}
	}

	sgct::MessageHandler::Instance()->print("Info: Allocated %d bytes for image data\n", mChannels * mSize_x * mSize_y);
}
