/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _IMAGE_H_
#define _IMAGE_H_

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/pngconf.h"
#else
#include <pngconf.h>
#endif

namespace sgct_core
{

class Image
{
public:
    enum ChannelType { Blue = 0, Green, Red, Alpha };
    
	Image();
	~Image();
	bool load(const char * filename);
	bool loadPNG(const char * filename);
	bool loadJPEG(const char * filename);
	bool save();
	bool savePNG(const char * filename, int compressionLevel = -1);
	bool savePNG(int compressionLevel = -1);
	bool saveJPEG(int quality = 100);
	bool saveTGA();
	void setFilename(const char * filename);
	void cleanup();
	unsigned char * getData();
	int getChannels();
	int getSizeX();
	int getSizeY();
    unsigned char getSampleAt(int x, int y, ChannelType c);
    float getInterpolatedSampleAt(float x, float y, ChannelType c);
	void setDataPtr(unsigned char * dPtr);
	void setSize(int width, int height);
	void setChannels(int channels);
	bool allocateOrResizeData();
	inline const char * getFilename() { return mFilename; }

private:
    bool isTGAPackageRLE(unsigned char * row, int pos);
    int getTGAPackageLength(unsigned char * row, int pos, bool rle);
    
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

