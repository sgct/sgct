/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__IMAGE__H__
#define __SGCT__IMAGE__H__

#include <pngconf.h>
#include <string>

namespace sgct::core {

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

    /// Load a jpeg compressed image from memory.
    bool loadJPEG(unsigned char* data, size_t len);
    bool loadTGA(std::string filename);
    bool loadTGA(unsigned char* data, size_t len);

    /// Save the buffer to file. Type is automatically set by filename suffix.
    bool save();

    /**
     * Compression levels 1-9.
     *   -1 = Default compression
     *    0 = No compression
     *    1 = Best speed
     *    9 = Best compression
     */
    bool savePNG(std::string filename, int compressionLevel = -1);
    bool savePNG(int compressionLevel = -1);
    bool saveJPEG(int quality = 100);
    bool saveTGA();
    void setFilename(std::string filename);
    void setPreferBGRExport(bool state);

    /**
     * Set if color pixel data should be stored as BGR(A) or RGB(A). BGR(A) is native for
     * most GPU hardware and is used as default.
     */
    void setPreferBGRImport(bool state);

    /**
     * Set if color pixel data should be stored as BGR(A) or RGB(A). BGR(A) is native for
     * most GPU hardware and is used as default.
     */
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

    /// Get sample from image data (all pixel values)
    unsigned char* getSampleAt(size_t x, size_t y);

    /// Set sample to image data (all pixel values)
    void setSampleAt(unsigned char* val, size_t x, size_t y);

    /// Get sample from image data. Only valid for 8-bit images
    unsigned char getSampleAt(size_t x, size_t y, ChannelType c);

    /// Set sample to image data
    void setSampleAt(unsigned char val, size_t x, size_t y, ChannelType c);

    /// Get interpolated sample from image data
    float getInterpolatedSampleAt(float x, float y, ChannelType c);

    void setDataPtr(unsigned char* dPtr);
    void setSize(size_t width, size_t height);
    void setChannels(size_t channels);
    void setBytesPerChannel(size_t bpc);
    const std::string& getFilename() const;

private:
    void cleanup();
    bool allocateRowPtrs();
    bool isTGAPackageRLE(unsigned char* row, size_t pos);
    bool decodeTGARLE(FILE* fp);
    bool decodeTGARLE(unsigned char* data, size_t len);
    size_t getTGAPackageLength(unsigned char* row, size_t pos, bool rle);
    
    size_t mChannels = 0;
    size_t mSizeX = 0;
    size_t mSizeY = 0;
    size_t mDataSize = 0;
    size_t mBytesPerChannel = 1;
    std::string mFilename;
    unsigned char* mData = nullptr;
    png_bytep* mRowPtrs = nullptr;
    bool mPreferBGRForExport = true;
    bool mPreferBGRForImport = true;
    bool mExternalData = false;
};

} // namespace sgct::core

#endif // __SGCT__IMAGE__H__
