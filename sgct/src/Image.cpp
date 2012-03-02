#include <stdio.h>
#include <fstream>
#include <png.h>
#ifdef __WIN32__
#include <pngpriv.h>
#endif

#include "../include/sgct/Image.h"
#include "../include/sgct/MessageHandler.h"

bool core_sgct::Image::load(const char * filename)
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

bool core_sgct::Image::loadPNG(const char *filename)
{
	mFilename = NULL;
	if( filename == NULL || strlen(filename) < 5) //one char + dot and suffix and is 5 char
	{
	    return false;
	}

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

	fread( header, 1, PNG_BYTES_TO_CHECK, fp );
	if( png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) )
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
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&size_x, (png_uint_32 *)&size_y, &bpp, &color_type, NULL, NULL, NULL);

	if(color_type == PNG_COLOR_TYPE_GRAY )
	{
		channels = 1;
		if(bpp < 8)
		{
			png_set_expand_gray_1_2_4_to_8(png_ptr);
			png_read_update_info(png_ptr, info_ptr);
		}
	}
	else if(color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		channels = 2;
	else if(color_type == PNG_COLOR_TYPE_RGB)
		channels = 3;
	else if(color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		channels = 4;
	else
	{
		sgct::MessageHandler::Instance()->print("Unsupported format '%s'\n", mFilename );
		fclose(fp);
		return false;
	}

	data = pb = (unsigned char*)malloc( sizeof(unsigned char)*( channels * size_x * size_y ) );
    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
	for( r = (int)png_get_image_height(png_ptr, info_ptr) - 1 ; r >= 0 ; r-- )
	{
		//png_bytep row = info_ptr->row_pointers[r];
        png_bytep row = row_pointers[r];
		int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
		int c;
		for( c = 0 ; c < rowbytes ; c++ )
			*(pb)++ = row[c];
			/*switch( channels )
			{
			case 1:
				*(pb)++ = row[c];
				break;
			case 2:
				sgct::MessageHandler::Instance()->print("Can't handle a two-channel PNG file: %s\n", mFilename );
				free(pb);
				fclose(fp);
				return false;
			case 3:
			case 4:
				*(pb)++ = row[c];
				break;
			}*/
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	fclose(fp);

	sgct::MessageHandler::Instance()->print("Image loaded %s\n", mFilename);
	return true;
}

void core_sgct::Image::cleanup()
{
	//delete data;
	free(data);
	sgct::MessageHandler::Instance()->print("Image data deleted %s\n", mFilename);

	delete [] mFilename;
	mFilename = NULL;
}

unsigned char * core_sgct::Image::getData()
{
	return data;
}
int core_sgct::Image::getChannels()
{
	return channels;
}

int core_sgct::Image::getSizeX()
{
	return size_x;
}

int core_sgct::Image::getSizeY()
{
	return size_y;
}
