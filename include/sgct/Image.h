/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__IMAGE__H__
#define __SGCT__IMAGE__H__

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/pngconf.h"
#else
#include <pngconf.h>
#endif

#include <string>

namespace sgct_core {

class Image {
public:
    enum ChannelType { Blue = 0, Green, Red, Alpha };
    enum class FormatType { PNG = 0, JPEG, TGA, Unknown };
    
    Image() = default;
    ~Image();

    bool allocateOrResizeData();
    bool load(std::string filename);
    bool loadPNG(std::string filename);
    bool loadPNG(unsigned char* data, size_t len);
    bool loadJPEG(std::string filename);
    bool loadJPEG(unsigned char* data, size_t len);
    bool loadTGA(std::string filename);
    bool loadTGA(unsigned char* data, size_t len);
    bool save();
    bool savePNG(std::string filename, int compressionLevel = -1);
    bool savePNG(int compressionLevel = -1);
    bool saveJPEG(int quality = 100);
    bool saveTGA();
    void setFilename(std::string filename);
    void setPreferBGRExport(bool state);
    void setPreferBGRImport(bool state);
    bool getPreferBGRExport() const;
    bool getPreferBGRImport() const;

    unsigned char* getData();
    const unsigned char* getData() const;
    unsigned char* getDataAt(size_t x, size_t y);
    size_t getChannels() const;
    size_t getWidth() const;
    size_t getHeight() const;
    size_t getDataSize() const;
    size_t getBytesPerChannel() const;

    unsigned char* getSampleAt(size_t x, size_t y);
    void setSampleAt(unsigned char* val, size_t x, size_t y);

    //only valid for 8-bit images
    unsigned char getSampleAt(size_t x, size_t y, ChannelType c);
    void setSampleAt(unsigned char val, size_t x, size_t y, ChannelType c);
    float getInterpolatedSampleAt(float x, float y, ChannelType c);
    

    void setDataPtr(unsigned char* dPtr);
    void setSize(size_t width, size_t height);
    void setChannels(size_t channels);
    void setBytesPerChannel(size_t bpc);
    const char* getFilename();

private:
    void cleanup();
    bool allocateRowPtrs();
    FormatType getFormatType(const std::string& filename);
    bool isTGAPackageRLE(unsigned char* row, size_t pos);
    bool decodeTGARLE(FILE* fp);
    bool decodeTGARLE(unsigned char* data, size_t len);
    std::size_t getTGAPackageLength(unsigned char* row, size_t pos, bool rle);
    
private:
    size_t mChannels = 0;
    size_t mSize_x = 0;
    size_t mSize_y = 0;
    size_t mDataSize = 0;
    size_t mBytesPerChannel = 1;
    std::string mFilename;
    unsigned char* mData = nullptr;
    png_bytep* mRowPtrs = nullptr;
    bool mPreferBGRForExport = true;
    bool mPreferBGRForImport = true;
    bool mExternalData = false;
};

} // namespace sgct_core

#endif // __SGCT__IMAGE__H__
