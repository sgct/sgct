/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

// png.h needs all of this included before its own include.. sigh
#include <stdio.h>
#include <fstream>
#include <algorithm>

#include <png.h>
#include <pngpriv.h>

#include <sgct/image.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <chrono>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {
    sgct::core::Image::FormatType getFormatType(std::string filename) {
        std::transform(
            filename.begin(),
            filename.end(),
            filename.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

        if (filename.find(".png") != std::string::npos) {
            return sgct::core::Image::FormatType::PNG;
        }
        if (filename.find(".jpg") != std::string::npos) {
            return sgct::core::Image::FormatType::JPEG;
        }
        if (filename.find(".jpeg") != std::string::npos) {
            return sgct::core::Image::FormatType::JPEG;
        }
        if (filename.find(".tga") != std::string::npos) {
            return sgct::core::Image::FormatType::TGA;
        }
        return sgct::core::Image::FormatType::Unknown;
    }
} // namespace

namespace sgct::core {

Image::~Image() {
    if (!_isExternalData && _data) {
        delete[] _data;
        _data = nullptr;
        _dataSize = 0;
    }
}

bool Image::load(const std::string& filename) {
    if (filename.empty()) {
        MessageHandler::printError("Image error: Cannot load empty filepath");
        return false;
    }

    stbi_set_flip_vertically_on_load(1);
    _data = stbi_load(filename.c_str(), &_size[0], &_size[1], &_nChannels, 0);
    _bytesPerChannel = 1;
    _dataSize = _size.x * _size.y * _nChannels * _bytesPerChannel;

    // Convert BGR to RGB
    if (_nChannels >= 3) {
        for (size_t i = 0; i < _dataSize; i += _nChannels) {
            unsigned char tmp = _data[i];
            _data[i] = _data[i + 2];
            _data[i + 2] = tmp;
        }
    }

    return _data != nullptr;
}

bool Image::load(unsigned char* data, int length) {
    stbi_set_flip_vertically_on_load(1);
    _data = stbi_load_from_memory(data, length, &_size[0], &_size[1], &_nChannels, 0);
    _bytesPerChannel = 1;
    _dataSize = _size.x * _size.y * _nChannels * _bytesPerChannel;

    // Convert BGR to RGB
    if (_nChannels >= 3) {
        for (size_t i = 0; i < _dataSize; i += _nChannels) {
            unsigned char tmp = _data[i];
            _data[i] = _data[i + 2];
            _data[i + 2] = tmp;
        }
    }

    return _data != nullptr;
}

bool Image::save(const std::string& filename) {
    if (filename.empty()) {
        MessageHandler::printError("Image error: Filename not set for saving image");
        return false;
    }

    FormatType type = getFormatType(filename);
    if (type == FormatType::Unknown) {
        // not found
        MessageHandler::printError("Image error: Cannot save file %s", filename.c_str());
        return false;
    }
    if (type == FormatType::PNG) {
        const bool success = savePNG(filename);
        return success;
    }

    if (_nChannels >= 3) {
        for (size_t i = 0; i < _dataSize; i += _nChannels) {
            unsigned char tmp = _data[i];
            _data[i] = _data[i + 2];
            _data[i + 2] = tmp;
        }
    }

    stbi_flip_vertically_on_write(1);
    if (type == FormatType::JPEG) {
        int r = stbi_write_jpg(
            filename.c_str(),
            _size.x,
            _size.y,
            _nChannels,
            _data,
            100
        );
        return r != 0;
    }
    if (type == FormatType::TGA) {
        int r = stbi_write_tga(filename.c_str(), _size.x, _size.y, _nChannels, _data);
        return r != 0;
    }

    // We should never get here unless someone added a type and forgot to save it
    assert(false);
    return false;
}

bool Image::savePNG(std::string filename, int compressionLevel) {
    if (_data == nullptr) {
        return false;
    }

    if (_bytesPerChannel > 2) {
        MessageHandler::printError(
            "Image error: Cannot save %d-bit PNG", _bytesPerChannel * 8
        );
        return false;
    }

    double t0 = Engine::getTime();
    
    FILE* fp = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s( &fp, filename.c_str(), "wb") != 0 || !fp) {
        MessageHandler::printError(
            "Image error: Can't create PNG file '%s'", filename.c_str()
        );
        return false;
    }
#else
    fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        MessageHandler::printError(
            "Image error: Can't create PNG file '%s'", filename.c_str()
        );
        return false;
    }
#endif

    // initialize stuff
    png_structp png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr,
        nullptr,
        nullptr
    );
    if (!png_ptr) {
        return false;
    }

    // set compression
    png_set_compression_level(png_ptr, compressionLevel);
    png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
    png_set_compression_mem_level(png_ptr, 8);
    png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
    png_set_compression_buffer_size(png_ptr, 8192);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    png_init_io(png_ptr, fp);

    int colorType = -1;
    switch (_nChannels) {
        case 1:
            colorType = PNG_COLOR_TYPE_GRAY;
            break;
        case 2:
            colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
            break;
        case 3:
            colorType = PNG_COLOR_TYPE_RGB;
            break;
        case 4:
            colorType = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
    }

    if (colorType == -1) {
        return false;
    }

    // write header
    png_set_IHDR(
        png_ptr,
        info_ptr,
        _size.x,
        _size.y,
        _bytesPerChannel * 8,
        colorType,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );
    
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
        png_set_bgr(png_ptr);
    }
    png_write_info(png_ptr, info_ptr);

    // write bytes
    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    // swap big-endian to little endian
    if (_bytesPerChannel == 2) {
        png_set_swap(png_ptr);
    }

    std::vector<png_bytep> rowPtrs(_size.y);

    for (int y = 0; y < _size.y; y++) {
        rowPtrs[(_size.y - 1) - y] = reinterpret_cast<png_bytep>(
            &_data[y * _size.x * _nChannels * _bytesPerChannel]
        );
    }
    png_write_image(png_ptr, rowPtrs.data());
    rowPtrs.clear();

    // end write
    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    MessageHandler::printDebug(
        "Image: '%s' was saved successfully (%.2f ms)",
        filename.c_str(), (Engine::getTime() - t0) * 1000.0
    );

    return true;
}

unsigned char* Image::getData() {
    return _data;
}

const unsigned char* Image::getData() const {
    return _data;
}

int Image::getChannels() const {
    return _nChannels;
}

int Image::getBytesPerChannel() const {
    return _bytesPerChannel;
}

glm::ivec2 Image::getSize() const {
    return _size;
}

void Image::setSize(glm::ivec2 size) {
    _size = std::move(size);
}

void Image::setChannels(int channels) {
    _nChannels = channels;
}

void Image::setBytesPerChannel(int bpc) {
    _bytesPerChannel = bpc;
}

bool Image::allocateOrResizeData() {
    double t0 = Engine::getTime();
    
    unsigned int dataSize = _nChannels * _size.x * _size.y * _bytesPerChannel;

    if (dataSize == 0) {
        MessageHandler::printError(
            "Image error: Invalid image size %dx%d %d channels",
            _size.x, _size.y, _nChannels
        );
        return false;
    }

    if (_data && _dataSize != dataSize) {
        // re-allocate if needed
        if (!_isExternalData && _data) {
            delete[] _data;
            _data = nullptr;
            _dataSize = 0;
        }
    }

    if (!_data) {
        _data = new unsigned char[dataSize];
        _dataSize = dataSize;
        _isExternalData = false;

        MessageHandler::printDebug(
            "Image: Allocated %d bytes for image data (%.2f ms)",
            _dataSize, (Engine::getTime() - t0) * 1000.0
        );
    }

    return true;
}

} // namespace sgct::core
