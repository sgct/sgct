#include <stdio.h>
#include <fstream>
#include <png.h>
#include <pngpriv.h>

#include "sgct/Image.h"

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
			fprintf( stderr, "Unknown filesuffix: \"%s\"\n", type, filename);
			return false;
		}
	}
	else
	{
		fprintf( stderr, "Image loading failed (bad filename: %s)\n", filename);
		return false;
	}
}

#define PNG_BYTES_TO_CHECK 8

bool core_sgct::Image::loadPNG(const char *filename)
{
	strcpy( _filename, filename );

	unsigned char *pb;
  
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	char header[PNG_BYTES_TO_CHECK];
	//int numChannels;
	int r, color_type, bpp;

	FILE *fp = fopen(_filename, "rb");
	if( !fp )
	{
		fprintf( stderr, "Can't open PNG texture file '%s'\n", _filename);
		return false;
	}

	fread( header, 1, PNG_BYTES_TO_CHECK, fp );
	if( png_sig_cmp( (png_byte*) &header[0], 0, PNG_BYTES_TO_CHECK) )
	{
		fprintf( stderr, "Texture file '%s' is not in PNG format\n", _filename);
		fclose(fp);
		return false;
	}

	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( png_ptr == NULL )
	{
		fprintf( stderr, "Can't initialize PNG file for reading: %s\n", _filename);
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if( info_ptr == NULL )
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		fprintf( stderr, "Can't allocate memory to read PNG file: %s\n", _filename);
		return false;
	}

	if( setjmp(png_jmpbuf(png_ptr)) )
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fp);
		fprintf( stderr, "Exception occurred while reading PNG file: %s\n", _filename);
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
		fprintf( stderr, "Unsupported format '%s'\n", _filename );
		fclose(fp);
		return false;
	}

	//size_x = png_get_image_width(png_ptr,info_ptr);
	//size_y = png_get_image_height(png_ptr,info_ptr);

	/*if( png_get_bit_depth(png_ptr, info_ptr) != 8 )
	{
		fprintf( stderr, "Can't handle PNG files with bit depth other than 8.  '%s' has %d bits per pixel", _filename, png_get_bit_depth(png_ptr, info_ptr) );
		fclose(fp);
		return;
	}
	if( channels == 2 )
	{
		fprintf( stderr, "Can't handle a two-channel PNG file: %s", _filename );
		fclose(fp);
		return;
	}*/

	data = pb = (unsigned char*)malloc( sizeof(unsigned char)*( channels * size_x * size_y ) );

	for( r = (int)info_ptr->height - 1 ; r >= 0 ; r-- )
	{
		png_bytep row = info_ptr->row_pointers[r];
		int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
		int c;
		for( c = 0 ; c < rowbytes ; c++ )
			switch( channels )
			{
			case 1:
				*(pb)++ = row[c];
				break;
			case 2:
				fprintf( stderr, "Can't handle a two-channel PNG file: %s\n", _filename );
				free(pb);
				fclose(fp);
				return false;
			case 3:
			case 4:
				*(pb)++ = row[c];
				break;
			}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	fclose(fp);
	
	fprintf( stderr, "Image loaded %s\n", _filename);
	return true;
}

void core_sgct::Image::cleanup()
{
	//delete data;
	free(data);
	fprintf( stderr, "Image data deleted %s\n", _filename);
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