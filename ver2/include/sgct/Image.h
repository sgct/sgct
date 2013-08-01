/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "external/pngconf.h"

namespace sgct_core
{

class Image
{
public:
	Image();
	~Image();
	bool load(const char * filename);
	bool loadPNG(const char * filename);
	bool save();
	bool savePNG(const char * filename, int compressionLevel = -1);
	bool savePNG(int compressionLevel = -1);
	bool saveTGA();
	void setFilename(const char * filename);
	void cleanup();
	unsigned char * getData();
	int getChannels();
	int getSizeX();
	int getSizeY();
	void setDataPtr(unsigned char * dPtr);
	void setSize(int width, int height);
	void setChannels(int channels);
	bool allocateOrResizeData();
	inline const char * getFilename() { return mFilename; }

private:
	int mChannels;
	int mSize_x;
	int mSize_y;
	char * mFilename;
	unsigned char * mData;
	png_bytep * mRowPtrs;
};

}

#endif

