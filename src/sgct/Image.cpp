/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <stdio.h>
#include <fstream>
#include <algorithm>

#ifndef SGCT_DONT_USE_EXTERNAL
#include "../include/external/png.h"
#include "../include/external/pngpriv.h"
#include "../include/external/jpeglib.h"
#include "../include/external/turbojpeg.h"
#else
#include <png.h>
#include <pngpriv.h>
#include <jpeglib.h>
#include <turbojpeg.h>
#endif
#include <stdlib.h>

#include "../include/sgct/Image.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/Engine.h"

#include <setjmp.h>

#define PNG_BYTES_TO_CHECK 8
size_t memOffset;

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

void readPNGFromBuffer(png_structp png_ptr, png_bytep outData, png_size_t length)
{
    if ( length <= 0 )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: PNG reading error! Invalid lenght.");
        return;
    }
        
    /* The file 'handle', a pointer, is stored in png_ptr->io_ptr */
    if ( png_ptr->io_ptr == NULL )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: PNG reading error! Invalid source pointer.");
        return;
    }
    
    if ( outData == NULL )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: PNG reading error! Invalid destination pointer.");
        return;
    }
            
    //copy buffer
    //faster more optimized copy
    png_size_t offset = 0;
    png_size_t stride = 4096;
    png_bytep src = reinterpret_cast<png_bytep>(png_ptr->io_ptr);
    
    while( offset < length )
    {
        if((length-offset) < stride)
            stride = length-offset;
        
        memcpy(outData + offset, src + offset + memOffset, stride);
        offset += stride;
    }
    
    memOffset += offset;
}

sgct_core::Image::Image()
{
	mData = NULL;
	mRowPtrs = NULL;
    
	mBytesPerChannel = 1;
    mChannels = 0;
	mSize_x = 0;
	mSize_y = 0;
	mDataSize = 0;
	mExternalData = false;
	mPreferBGRForExport = true;
}

sgct_core::Image::~Image()
{
	cleanup();
}

sgct_core::Image::FormatType sgct_core::Image::getFormatType(const std::string & filename)
{
	std::string filenameLC;
	filenameLC.resize(filename.size());

	std::transform(filename.begin(), filename.end(), filenameLC.begin(), ::tolower);

	std::size_t found;

	//if png file
	found = filenameLC.find(".png");
	if (found != std::string::npos)
	{
		return FORMAT_PNG;
	}

	//if jpg
	found = filenameLC.find(".jpg");
	if (found != std::string::npos)
	{
		return FORMAT_JPEG;
	}

	//if jpeg
	found = filenameLC.find(".jpeg");
	if (found != std::string::npos)
	{
		return FORMAT_JPEG;
	}

	//if tga
	found = filenameLC.find(".tga");
	if (found != std::string::npos)
	{
		return FORMAT_TGA;
	}

	//no match found
	return UNKNOWN_FORMAT;
}

bool sgct_core::Image::load(std::string filename)
{
	if (filename.empty())
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Cannot load emtpy filepath!\n");
		return false;
	}

	switch (getFormatType(filename))
	{
	case FORMAT_PNG:
		loadPNG(filename);
		return true;
		break;

	case FORMAT_JPEG:
		loadJPEG(filename);
		return true;
		break;

	default:
		//not found
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Unknown file '%s'\n", filename.c_str());
		return false;
		break;
	}
}

bool sgct_core::Image::loadJPEG(std::string filename)
{
	if (filename.empty()) //one char + dot and suffix and is 5 char
	{
		return false;
	}

	mFilename.assign(filename);
	
	struct my_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;
	FILE * fp = NULL;
	JSAMPARRAY buffer;
	int row_stride;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&fp, mFilename.c_str(), "rb") != 0 || !fp)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open JPEG texture file '%s'\n", mFilename.c_str());
		return false;
	}
#else
	fp = fopen(mFilename.c_str(), "rb");
	if (fp == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open JPEG texture file '%s'\n", mFilename.c_str());
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open JPEG texture file '%s'\n", mFilename.c_str());
		return false;
	}
	
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	row_stride = cinfo.output_width * cinfo.output_components;

	//SGCT uses BGR so convert to that
	cinfo.out_color_space = JCS_EXT_BGR;

	mBytesPerChannel = 1; //only support 8-bit per color depth for jpeg even if the format supports up to 12-bit
	mChannels = cinfo.output_components;
	mSize_x = cinfo.output_width;
	mSize_y = cinfo.output_height;
	
	if (!allocateOrResizeData())
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
		return false;
	}

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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: Loaded %s (%dx%d).\n", mFilename.c_str(), mSize_x, mSize_y);

	return true;
}

/*!
 Load a jpeg compressed image from memory.
 */
bool sgct_core::Image::loadJPEG(unsigned char * data, int len)
{
    if(data == NULL || len <= 0)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: failed to load JPEG from memory. Invalid input data.");
        return false;
    }
    
    tjhandle turbo_jpeg_handle = tjInitDecompress();
    
    int jpegsubsamp;
    int pixelformat;

	mBytesPerChannel = 1; //only support 8-bit per color depth for jpeg even if the format supports up to 12-bit
    
    /*
     use tjDecompressHeader3 for newer versions of turbo-jpeg.
     */
    
    if( tjDecompressHeader2(turbo_jpeg_handle, data, static_cast<unsigned long>(len), &mSize_x, &mSize_y, &jpegsubsamp) < 0 )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: failed to load JPEG from memory. Error: %s!\n", tjGetErrorStr());
        tjDestroy(turbo_jpeg_handle);
        return false;
    }
    
    switch(jpegsubsamp)
    {
        case TJSAMP_444:
        case TJSAMP_422:
        case TJSAMP_420:
        case TJSAMP_440:
            mChannels = 3;
            pixelformat = TJPF_BGR;
            break;
            
        case TJSAMP_GRAY:
            mChannels = 1;
            pixelformat = TJPF_GRAY;
            break;

        default:
            mChannels = -1;
            break;
    }
    
    if(mChannels < 1)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: failed to load JPEG from memory. Unsupported chrominance subsampling!\n");
        tjDestroy(turbo_jpeg_handle);
        return false;
    }
    
	if (!allocateOrResizeData())
	{
		tjDestroy(turbo_jpeg_handle);
		return false;
	}

    if( !mData )
    {
        tjDestroy(turbo_jpeg_handle);
        return false;

    }
    
    if( tjDecompress2(turbo_jpeg_handle, data, static_cast<unsigned long>(len), mData, mSize_x, 0, mSize_y, pixelformat, TJFLAG_FASTDCT) < 0 )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: failed to load JPEG from memory. Error: %s!\n", tjGetErrorStr());
        tjDestroy(turbo_jpeg_handle);
        
        delete [] mData;
        mData = NULL;
        
        return false;
    }
    
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: Loaded %dx%d JPEG from memory.\n", mSize_x, mSize_y);
    
    tjDestroy(turbo_jpeg_handle);
    return true;
}

bool sgct_core::Image::loadPNG(std::string filename)
{
	if (filename.empty()) //one char + dot and suffix and is 5 char
	{
		return false;
	}

	mFilename.assign(filename);

	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char header[PNG_BYTES_TO_CHECK];
	//int numChannels;
	int r, color_type, bpp;

	FILE *fp = NULL;
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&fp, mFilename.c_str(), "rb") != 0 || !fp)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open PNG texture file '%s'\n", mFilename.c_str());
		return false;
	}
    #else
	fp = fopen(mFilename.c_str(), "rb");
    if( fp == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't open PNG texture file '%s'\n", mFilename.c_str());
		return false;
	}
    #endif

	size_t result = fread( header, 1, PNG_BYTES_TO_CHECK, fp );
	if( result != PNG_BYTES_TO_CHECK || png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: file '%s' is not in PNG format\n", mFilename.c_str());
		fclose(fp);
		return false;
	}

	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( png_ptr == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't initialize PNG file for reading: %s\n", mFilename.c_str());
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr == NULL )
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't allocate memory to read PNG file: %s\n", mFilename.c_str());
		return false;
	}

	if( setjmp(png_jmpbuf(png_ptr)) )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Exception occurred while reading PNG file: %s\n", mFilename.c_str());
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

	png_read_png( png_ptr, info_ptr,
                //PNG_TRANSFORM_STRIP_16 | //we want 16-bit support
				PNG_TRANSFORM_SWAP_ENDIAN | //needed for 16-bit support
                PNG_TRANSFORM_PACKING |
                PNG_TRANSFORM_EXPAND | 
				PNG_TRANSFORM_BGR, NULL);
	png_set_bgr(png_ptr);

	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&mSize_x, (png_uint_32 *)&mSize_y, &bpp, &color_type, NULL, NULL, NULL);
	png_set_bgr(png_ptr);

	mBytesPerChannel = bpp / 8;

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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Unsupported format '%s'\n", mFilename.c_str());
		fclose(fp);
		return false;
	}

	if (!allocateOrResizeData())
	{
		fclose(fp);
		return false;
	}

	unsigned char * pb = mData;

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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: Loaded %s (%dx%d %d-bit).\n", mFilename.c_str(), mSize_x, mSize_y, mBytesPerChannel * 8);

	return true;
}

bool sgct_core::Image::loadPNG(unsigned char * data, int len)
{
    if(data == NULL || len <= PNG_BYTES_TO_CHECK)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image: failed to load PNG from memory. Invalid input data.");
        return false;
    }
    
    png_structp png_ptr;
	png_infop info_ptr;
	unsigned char header[PNG_BYTES_TO_CHECK];
	//int numChannels;
	int r, color_type, bpp;
    
	//get header
    memcpy( header, data, PNG_BYTES_TO_CHECK);
    if(!png_check_sig( header, PNG_BYTES_TO_CHECK))
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Invalid PNG file header.\n");
        return false;
    }
    
	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( png_ptr == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't initialize PNG.\n");
		return false;
	}
    
	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr == NULL )
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't allocate memory to read PNG data.\n");
		return false;
	}
    
    //set the read position in memory
    memOffset = PNG_BYTES_TO_CHECK;
    png_set_read_fn(png_ptr, data, readPNGFromBuffer);
    
	if( setjmp(png_jmpbuf(png_ptr)) )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Exception occurred while reading PNG data.\n");
		return false;
	}
    
    png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
    
    png_read_png( png_ptr, info_ptr,
                 PNG_TRANSFORM_STRIP_16 |
                 PNG_TRANSFORM_PACKING |
                 PNG_TRANSFORM_EXPAND |
                 PNG_TRANSFORM_BGR, NULL);
	png_set_bgr(png_ptr);
    
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&mSize_x, (png_uint_32 *)&mSize_y, &bpp, &color_type, NULL, NULL, NULL);
	png_set_bgr(png_ptr);

	mBytesPerChannel = bpp / 8;
    
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Unsupported format!\n" );
		return false;
	}
    
	if (!allocateOrResizeData())
	{
		return false;
	}

	unsigned char * pb = mData;

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
    
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Image: Loaded %dx%d %d-bit PNG from memory.\n", mSize_x, mSize_y, mBytesPerChannel*8);
    
    return true;
}

/*!
	Save the buffer to file. Type is automatically set by filename suffix.
*/
bool sgct_core::Image::save()
{
	if(mFilename.empty())
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Filename not set for saving image.\n");
		return false;
	}

	switch (getFormatType(mFilename))
	{
	case FORMAT_PNG:
		savePNG();
		return true;
		break;

	case FORMAT_JPEG:
		saveJPEG();
		return true;
		break;

	case FORMAT_TGA:
		saveTGA();
		return true;
		break;

	default:
		//not found
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Cannot save file '%s'\n", mFilename.c_str());
		return false;
		break;
	}
}

/*!
		Compression levels 1-9.
		-1 = Default compression
		0 = No compression
		1 = Best speed
		9 = Best compression
*/
bool sgct_core::Image::savePNG(std::string filename, int compressionLevel)
{
	setFilename( filename );
	return savePNG( compressionLevel );
}

bool sgct_core::Image::savePNG(int compressionLevel)
{
	if( mData == NULL )
		return false;

	if (mBytesPerChannel > 2)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Cannot save %d-bit PNG.\n", mBytesPerChannel * 8);
		return false;
	}

	double t0 = sgct::Engine::getTime();
    
    FILE *fp = NULL;
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( fopen_s( &fp, mFilename.c_str(), "wb") != 0 || !fp )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create PNG file '%s'\n", mFilename.c_str());
		return false;
	}
    #else
	fp = fopen(mFilename.c_str(), "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create PNG file '%s'\n", mFilename.c_str());
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
		mBytesPerChannel*8, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	if (mPreferBGRForExport && (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA))
		png_set_bgr(png_ptr);
	png_write_info(png_ptr, info_ptr);

	/* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
		return false;

	//swap big-endian to little endian
	if (mBytesPerChannel == 2)
		png_set_swap(png_ptr);

	for (int y = (mSize_y-1);  y >= 0;  y--)
		mRowPtrs[(mSize_y - 1) - y] = (png_bytep)&mData[y * mSize_x * mChannels * mBytesPerChannel];
    png_write_image(png_ptr, mRowPtrs);

	/* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
		return false;

    png_write_end(png_ptr, NULL);

	png_destroy_write_struct (&png_ptr, &info_ptr);

	fclose(fp);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Image: '%s' was saved successfully (%.2f ms)!\n", mFilename.c_str(), (sgct::Engine::getTime() - t0)*1000.0);

	return true;
}

bool sgct_core::Image::saveJPEG(int quality)
{
	if (mData == NULL)
		return false;

	if (mBytesPerChannel > 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Cannot save %d-bit JPEG.\n", mBytesPerChannel * 8);
		return false;
	}

	double t0 = sgct::Engine::getTime();

	FILE *fp = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&fp, mFilename.c_str(), "wb") != 0 || !fp)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create JPEG file '%s'\n", mFilename.c_str());
		return false;
	}
#else
	fp = fopen(mFilename.c_str(), "wb");
	if (fp == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create JPEG file '%s'\n", mFilename.c_str());
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
		cinfo.in_color_space = mPreferBGRForExport ? JCS_EXT_BGRA : JCS_EXT_RGBA;
		break;

	case 3:
	default:
		cinfo.in_color_space = mPreferBGRForExport ? JCS_EXT_BGR : JCS_RGB;
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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Image: '%s' was saved successfully (%.2f ms)!\n", mFilename.c_str(), (sgct::Engine::getTime() - t0)*1000.0);
	return true;
}

bool sgct_core::Image::saveTGA()
{
	if( mData == NULL )
		return false;

	if (mBytesPerChannel > 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Cannot save %d-bit TGA.\n", mBytesPerChannel * 8);
		return false;
	}
    
    double t0 = sgct::Engine::getTime();

	FILE *fp = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&fp, mFilename.c_str(), "wb") != 0 || !fp)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create TGA texture file '%s'\n", mFilename.c_str());
		return false;
	}
#else
	fp = fopen(mFilename.c_str(), "wb");
    if( fp == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create TGA texture file '%s'\n", mFilename.c_str());
		return false;
	}
#endif

	if( mChannels == 2 )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Can't create TGA texture file '%s'.\nLuminance alpha not supported by the TGA format.\n", mFilename.c_str());
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

	// convert the image data from RGB(a) to BGR(A)
	if (!mPreferBGRForExport)
	{
		unsigned char tmp;
		if (mChannels >= 3)
			for (int i = 0; i < mDataSize; i += mChannels)
			{
				tmp = mData[i];
				mData[i] = mData[i + 2];
				mData[i + 2] = tmp;
			}
	}

    //write row-by-row
    if( data_type != 10 ) //Non RLE compression
    {
        for(int y=0; y<mSize_y; y++)
			fwrite(&mData[y * mSize_x * mChannels], mChannels, mSize_x, fp);
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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Image: '%s' was saved successfully (%.2f ms)!\n", mFilename.c_str(), (sgct::Engine::getTime() - t0)*1000.0);

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

void sgct_core::Image::setFilename(std::string filename)
{

	if( filename.empty() || filename.length() < 5) //one char + dot and suffix and is 5 char
	{
	    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Invalid filename!\n");
		return;
	}

	mFilename.assign(filename);
}

/*!
Set if color pixel data should be stored as BGR(A) or RGB(A). BGR(A) is native for most GPU hardware and is used as default.
*/
void sgct_core::Image::setPreferBGRExport(bool state)
{
	mPreferBGRForExport = state;
}

void sgct_core::Image::cleanup()
{
	if (!mExternalData && mData)
	{
		delete [] mData;
		mData = NULL;
		mDataSize = 0;
	}

	if(mRowPtrs)
	{
		delete [] mRowPtrs;
		mRowPtrs = NULL;
	}
}

unsigned char * sgct_core::Image::getData()
{
	return mData;
}
int sgct_core::Image::getChannels() const
{
	return mChannels;
}

int sgct_core::Image::getWidth() const
{
	return mSize_x;
}

int sgct_core::Image::getHeight() const
{
	return mSize_y;
}

int sgct_core::Image::getDataSize() const
{
	return mDataSize;
}

int sgct_core::Image::getBytesPerChannel() const
{
	return mBytesPerChannel;
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
	if (!mExternalData && mData)
	{
		delete[] mData;
		mData = NULL;
		mDataSize = 0;
	}

	allocateRowPtrs();
	
	mData = dPtr;
	mExternalData = true;
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

void sgct_core::Image::setBytesPerChannel(int bpc)
{
	mBytesPerChannel = bpc;
}

bool sgct_core::Image::allocateOrResizeData()
{
	int dataSize = mChannels * mSize_x * mSize_y * mBytesPerChannel;

	if (dataSize <= 0)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Invalid image size %dx%d %d channels!\n",
			mSize_x, mSize_y, mChannels);
		return false;
	}

	if (mData && mDataSize != dataSize) //re-allocate if needed
	{
		cleanup();
	}

	if ( !mData )
	{
		try
		{
			mData = new unsigned char[dataSize];
			mDataSize = dataSize;
			mExternalData = false;
		}
		catch (std::bad_alloc& ba)
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Failed to allocate %d bytes of image data (%s).\n", dataSize, ba.what());
			mData = NULL;
			mDataSize = 0;
			return false;
		}

		if (!allocateRowPtrs())
			return false;

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Image: Allocated %d bytes for image data\n", mDataSize);
	}

	return true;
}

bool sgct_core::Image::allocateRowPtrs()
{
	if (mRowPtrs)
	{
		delete[] mRowPtrs;
		mRowPtrs = NULL;
	}

	try
	{
		mRowPtrs = new png_bytep[mSize_y];
	}
	catch (std::bad_alloc& ba)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Failed to allocate pointers for image data (%s).\n", ba.what());
		mRowPtrs = NULL;
		return false;
	}

	return true;
}
