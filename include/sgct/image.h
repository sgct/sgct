/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__IMAGE__H__
#define __SGCT__IMAGE__H__

#include <string>
#include <glm/glm.hpp>

namespace sgct::core {

class Image {
public:
    enum class FormatType { PNG = 0, JPEG, TGA, Unknown };
    
    Image() = default;
    ~Image();

    bool allocateOrResizeData();
    bool load(const std::string& filename);

    /// Save the buffer to file. Type is automatically set by filename suffix.
    bool save(const std::string& filename);

    unsigned char* getData();
    const unsigned char* getData() const;
    int getChannels() const;
    glm::ivec2 getSize() const;

    void setSize(glm::ivec2 size);
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
    bool savePNG(std::string filename, int compressionLevel = -1);

    int _nChannels = 0;
    glm::ivec2 _size;
    unsigned int _dataSize = 0;
    int _bytesPerChannel = 1;
    unsigned char* _data = nullptr;
    bool _isExternalData = false;
};

} // namespace sgct::core

#endif // __SGCT__IMAGE__H__
