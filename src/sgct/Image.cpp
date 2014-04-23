/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <stdio.h>
#include <fstream>
#ifndef SGCT_DONT_USE_EXTERNAL
#include "../include/external/png.h"
#include "../include/external/pngpriv.h"
#include "../include/external/jpeglib.h"
#else
#include <png.h>
#include <pngpriv.h>
#include <jpeglib.h>
#endif
#include <stdlib.h>

#include "../include/sgct/Image.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/Engine.h"

#include <setjmp.h>

#define PNG_BYTES_TO_CHECK 8

//---------------- JPEG helpers -----------------
struct my_error_mgr
{
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
* Here's the routine that will replace the standard error_exit method:
*/

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

sgct_core::Image::Image()
{
	mFilename = NULL;
	mData = NULL;
	mRowPtrs = NULL;
    
    mChannels = 0;
	mSize_x = 0;
	mSize_y = 0;
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
		else if (strcmp(".JPG", type) == 0 || strcmp(".jpg", type) == 0)
			return loadJPEG(filename);
		else
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Unknown filesuffix: \"%s\"\n", type);
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Loading failed (bad filename: %s)\n", filename);
		return false;
	}
}

bool sgct_core::Image::loadJPEG(const char * filename)
{
	mFilename = NULL;
	if (filename == NULL || strlen(filename) < 5) //one char + dot and suffix and is 5 char
	{
		return false;
	}

	//copy filename
	mFilename = new char[strlen(filename) + 1];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (strcpy_s(mFilename, strlen(filename) + 1, filename) != 0)
		return false;
#else
	strcpy(mFilename, filename);
#endif
	
	struct my_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;
	FILE * fp = NULL;
	JSAMPARRAY buffer;
	int row_stride;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&fp, mFilename, "rb") != 0 || !fp)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open JPEG texture file '%s'\n", mFilename);
		return false;
	}
#else
	fp = fopen(mFilename, "rb");
	if (fp == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open JPEG texture file '%s'\n", mFilename);
		return false;
	}
#endif

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open JPEG texture file '%s'\n", mFilename);
		return false;
	}
	
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	row_stride = cinfo.output_width * cinfo.output_components;

	//SGCT uses BGR so convert to that
	cinfo.out_color_space = JCS_EXT_BGR;

	mChannels = cinfo.output_components;
	mSize_x = cinfo.output_width;
	mSize_y = cinfo.output_height;
	mData = new unsigned char[mChannels * mSize_x * mSize_y];

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

	int r = mSize_y-1;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, buffer, 1);
		//flip vertically
		memcpy(mData + row_stride*r, buffer[0], row_stride);
		r--;
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(fp);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: Loaded %s.\n", mFilename);

	//clean up filename
	if (mFilename != NULL)
	{
		delete[] mFilename;
		mFilename = NULL;
	}

	return true;
}

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
    if( fopen_s( &fp, mFilename, "rb") != 0 || !fp )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open PNG texture file '%s'\n", mFilename);
		return false;
	}
    #else
    fp = fopen(mFilename, "rb");
    if( fp == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open PNG texture file '%s'\n", mFilename);
		return false;
	}
    #endif

	size_t result = fread( header, 1, PNG_BYTES_TO_CHECK, fp );
	if( result != PNG_BYTES_TO_CHECK || png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Texture file '%s' is not in PNG format\n", mFilename);
		fclose(fp);
		return false;
	}

	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( png_ptr == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't initialize PNG file for reading: %s\n", mFilename);
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr == NULL )
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't allocate memory to read PNG file: %s\n", mFilename);
		return false;
	}

	if( setjmp(png_jmpbuf(png_ptr)) )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Exception occurred while reading PNG file: %s\n", mFilename);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	png_read_png( png_ptr, info_ptr,
                PNG_TRANSFORM_STRIP_16 |
                PNG_TRANSFORM_PACKING |
                PNG_TRANSFORM_EXPAND | 
				PNG_TRANSFORM_BGR, NULL);
	png_set_bgr(png_ptr);

	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&mSize_x, (png_uint_32 *)&mSize_y, &bpp, &color_type, NULL, NULL, NULL);
	png_set_bgr(png_ptr);

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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Unsupported format '%s'\n", mFilename );
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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: Loaded %s.\n", mFilename);

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
	//test
	//return true;
	
	int length = 0;

	if(mFilename == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Filename not set for saving image.\n");
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
			return savePNG( sgct::SGCTSettings::instance()->getPNGCompressionLevel() );
		else if( strcmp(".TGA", type) == 0 || strcmp(".tga", type) == 0 )
			return saveTGA();
		else if (strcmp(".JPG", type) == 0 || strcmp(".jpg", type) == 0)
			return saveJPEG( sgct::SGCTSettings::instance()->getJPEGQuality() );
		else
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Failed to save image! Unknown filesuffix: \"%s\"\n", type);
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Failed to save image! Bad filename: %s\n", mFilename);
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

	double t0 = sgct::Engine::getTime();
    
    FILE *fp = NULL;
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename, "wb") != 0 || !fp )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create PNG texture file '%s'\n", mFilename);
		return false;
	}
    #else
    fp = fopen(mFilename, "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create PNG texture file '%s'\n", mFilename);
		return false;
	}
    #endif

	/* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
		return false;

	//set compression
	png_set_compression_level( png_ptr, compressionLevel );
	//png_set_filter(png_ptr, 0, PNG_FILTER_NONE );
    
    png_set_filter(png_ptr, 0, PNG_FILTER_NONE );
    
    png_set_compression_mem_level(png_ptr, 8);
    //png_set_compression_mem_level(png_ptr, MAX_MEM_LEVEL);
	//png_set_compression_strategy(png_ptr, Z_HUFFMAN_ONLY);
    
	sgct::SGCTSettings::instance()->getUseRLE() ? 
		png_set_compression_strategy(png_ptr, Z_RLE) :
		png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
    png_set_compression_buffer_size(png_ptr, 8192);

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
	
	if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		png_set_bgr(png_ptr);
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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: '%s' was saved successfully (%.2f ms)!\n", mFilename, (sgct::Engine::getTime() - t0)*1000.0);

	return true;
}

bool sgct_core::Image::saveJPEG(int quality)
{
	if (mData == NULL && !allocateOrResizeData())
		return false;

	double t0 = sgct::Engine::getTime();

	FILE *fp = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&fp, mFilename, "wb") != 0 || !fp)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create JPEG texture file '%s'\n", mFilename);
		return false;
	}
#else
	fp = fopen(mFilename, "wb");
	if (fp == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create JPEG texture file '%s'\n", mFilename);
		return false;
	}
#endif
	
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
	int row_stride;	/* physical row width in image buffer */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = mSize_x;
	cinfo.image_height = mSize_y;
	cinfo.input_components = mChannels;

	switch (mChannels)
	{
	case 4:
		cinfo.in_color_space = JCS_EXT_BGRA;
		break;

	case 3:
	default:
		cinfo.in_color_space = JCS_EXT_BGR;
		break;

	case 2:
		cinfo.in_color_space = JCS_UNKNOWN;
		break;

	case 1:
		cinfo.in_color_space = JCS_GRAYSCALE;
		break;
	}

	if (cinfo.in_color_space == JCS_UNKNOWN)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: JPEG doesn't support two channel output!\n");
		return false;
	}

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = mSize_x * mChannels;	/* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height)
	{
		//flip vertically
		row_pointer[0] = &mData[(mSize_y - cinfo.next_scanline - 1) * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(fp);

	jpeg_destroy_compress(&cinfo);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: '%s' was saved successfully (%.2f ms)!\n", mFilename, (sgct::Engine::getTime() - t0)*1000.0);
	return true;
}

bool sgct_core::Image::saveTGA()
{
	if( mData == NULL && !allocateOrResizeData())
		return false;
    
    double t0 = sgct::Engine::getTime();

	FILE *fp = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename, "wb") != 0 || !fp )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create TGA texture file '%s'\n", mFilename);
		return false;
	}
#else
    fp = fopen(mFilename, "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create TGA texture file '%s'\n", mFilename);
		return false;
	}
#endif

	if( mChannels == 2 )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create TGA texture file '%s'.\nLuminance alpha not supported by the TGA format.\n", mFilename);
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
		data_type = sgct::SGCTSettings::instance()->getUseRLE() ? 10 : 2;
		//data_type = 2;//uncompressed RGB
        //data_type = 10;//RLE compressed RGB
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

    //write row-by-row
    if( data_type != 10 ) //Non RLE compression
    {
        for(int y=0; y<mSize_y; y++)
            fwrite( &mData[y * mSize_x * mChannels], mChannels, mSize_x, fp);
    }
    else //RLE ->only for RBG and minimum size is 3x3
    {
        for(int y=0; y<mSize_y; y++)
        {
            int pos = 0;
            unsigned char * row;
            
            while (pos < mSize_y)
            {
                row = &mData[y * mSize_x * mChannels];
                bool rle = isTGAPackageRLE(row, pos);
                int len = getTGAPackageLength(row, pos, rle);
                
                unsigned char packetHeader = static_cast<unsigned char>(len) - 1;
                
                if (rle)
                    packetHeader |= (1 << 7);
                
                fwrite( &packetHeader, 1, 1, fp);
                
                rle ?
                    fwrite( row + pos * mChannels, mChannels, 1, fp) :
                    fwrite( row + pos * mChannels, mChannels, len, fp);
                
                pos += len;

            }
        }//end for
    }//end if RLE

	fwrite(footer, sizeof(char), sizeof(footer), fp);

	fclose(fp);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: '%s' was saved successfully (%.2f ms)!\n", mFilename, (sgct::Engine::getTime() - t0)*1000.0);

	return true;
}

bool sgct_core::Image::isTGAPackageRLE(unsigned char * row, int pos)
{
    if (pos == mSize_x - 1)
        return false;
    
    unsigned char * p0 = row + pos * mChannels;
    unsigned char * p1 = p0 + mChannels;
    
    //minimum three same pixels in row
    return ((pos < mSize_x - 2) && memcmp(p0, p1, mChannels) == 0 && memcmp(p1, p1 + mChannels, mChannels) == 0);
}

int sgct_core::Image::getTGAPackageLength(unsigned char * row, int pos, bool rle)
{
    if (mSize_x - pos < 3)
        return mSize_x - pos;
    
    /*if (pos == mSize_x - 1)
        return 1;
    
    if (pos == mSize_x - 2)
        return 2;*/
    
    int len = 2;
    if (rle)
    {
        while (pos + len < mSize_x)
        {
            if ( memcmp(&row[pos * mChannels], &row[(pos+len) * mChannels], mChannels) == 0 )
                len++;
            else
                return len;
            
            if (len == 128)
                return 128;
        }
    }
    else
    {
        while (pos + len < mSize_x)
        {
            if(isTGAPackageRLE(row, pos+len))
               return len;
            else
               len++;
            
            if (len == 128)
                return 128;
        }
    }
    return len;
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
	    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Invalid filename!\n");
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

/*!
Get sample from image data
*/
unsigned char sgct_core::Image::getSampleAt(int x, int y, sgct_core::Image::ChannelType c)
{
    return mData[(y * mSize_x + x) * mChannels + c];
}

/*!
 Get interpolated sample from image data
*/
float sgct_core::Image::getInterpolatedSampleAt(float x, float y, sgct_core::Image::ChannelType c)
{
    int px = static_cast<int>(x); //floor x
    int py = static_cast<int>(y); //floor y
    
    // Calculate the weights for each pixel
    float fx = x - static_cast<float>(px);
    float fy = y - static_cast<float>(py);
    
    //if no need for interpolation
    if(fx == 0.0f && fy == 0.0f)
        return static_cast<float>( getSampleAt(px, py, c) );
    
    float fx1 = 1.0f - fx;
    float fy1 = 1.0f - fy;
    
    float w0 = fx1 * fy1;
    float w1 = fx  * fy1;
    float w2 = fx1 * fy;
    float w3 = fx  * fy;
    
    float p0 = static_cast<float>( getSampleAt(px, py, c) );
    float p1 = static_cast<float>( getSampleAt(px, py+1, c) );
    float p2 = static_cast<float>( getSampleAt(px+1, py, c) );
    float p3 = static_cast<float>( getSampleAt(px+1, py+1, c) );
    
    return p0 * w0 + p1 * w1 + p2 * w2 + p3 * w3;
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Invalid image size %dx%d %d channels!\n",
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Failed to allocate %d bytes of image data (%s).\n", mChannels * mSize_x * mSize_y, ba.what());
		mData = NULL;
		return false;
	}

	try
	{
		mRowPtrs = new png_bytep[ mSize_y ];
	}
	catch(std::bad_alloc& ba)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Failed to allocate pointers for image data (%s).\n", ba.what());
		mRowPtrs = NULL;
		return false;
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: Allocated %d bytes for image data\n", mChannels * mSize_x * mSize_y);
	return true;
}
