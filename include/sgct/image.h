/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__IMAGE__H__
#define __SGCT__IMAGE__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <string>

namespace sgct {

class SGCT_EXPORT Image {
public:
    enum class FormatType { PNG = 0, JPEG, TGA, Unknown };

    Image() = default;
    ~Image();

    void allocateOrResizeData();
    void load(const std::string& filename);
    void load(unsigned char* data, int length);

    /**
     * Save the buffer to file. Type is automatically set by filename suffix.
     */
    void save(const std::string& filename);

    unsigned char* data();
    const unsigned char* data() const;
    int channels() const;
    int bytesPerChannel() const;
    ivec2 size() const;

    void setSize(ivec2 size);
    void setChannels(int channels);
    void setBytesPerChannel(int bpc);

private:
    /**
     * Compression levels 1-9.
     *   -1 = Default compression
     *    0 = No compression
     *    1 = Best speed
     *    9 = Best compression
     */
    void savePNG(std::string filename, int compressionLevel = -1);

    int _nChannels = 0;
    ivec2 _size = ivec2{ 0, 0 };
    unsigned int _dataSize = 0;
    int _bytesPerChannel = 1;
    unsigned char* _data = nullptr;
};

} // namespace sgct

#endif // __SGCT__IMAGE__H__
