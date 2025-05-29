/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/image.h>

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <png.h>
#include <zlib.h>
#include <algorithm>
#include <chrono>
#include <string>

#ifdef WIN32
#include <CodeAnalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif // WIN32

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif // __clang__

namespace {
#define STBI_NO_SIMD
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
} // namespace

namespace {
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
} // namespace

#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__
#pragma GCC diagnostic pop
#endif // __clang__

#ifdef WIN32
#pragma warning(pop)
#pragma warning(disable : 4611)
#endif // WIN32

#define Err(code, msg) Error(Error::Component::Image, code, msg)

namespace sgct {

Image::~Image() {
    if (_data) {
        stbi_image_free(_data);
    }
}

void Image::load(const std::filesystem::path& filename) {
    if (filename.empty()) {
        throw Err(9000, "Cannot load empty filepath");
    }

    stbi_set_flip_vertically_on_load(1);
    std::string name = filename.string();
    _data = stbi_load(name.c_str(), &_size.x, &_size.y, &_nChannels, 0);
    if (_data == nullptr) {
        throw Err(
            9001, std::format("Could not open file '{}' for loading image", filename.string()));
    }
    _bytesPerChannel = 1;
    _dataSize = _size.x * _size.y * _nChannels * _bytesPerChannel;

    // Convert BGR to RGB
    if (_nChannels >= 3) {
        for (size_t i = 0; i < _dataSize; i += _nChannels) {
            std::swap(_data[i], _data[i + 2]);
        }
    }
}

void Image::load(unsigned char* data, int length) {
    stbi_set_flip_vertically_on_load(1);
    _data = stbi_load_from_memory(data, length, &_size.x, &_size.y, &_nChannels, 0);
    _bytesPerChannel = 1;
    _dataSize = _size.x * _size.y * _nChannels * _bytesPerChannel;

    // Convert BGR to RGB
    if (_nChannels >= 3) {
        for (size_t i = 0; i < _dataSize; i += _nChannels) {
            std::swap(_data[i], _data[i + 2]);
        }
    }
}

void Image::save(const std::filesystem::path& filename) {
    if (filename.empty()) {
        throw Err(9002, "Filename not set for saving image");
    }

    // We use libPNG instead of stb as libPNG is faster and we care about how fast
    // PNGs are written to disk in production
    if (_data == nullptr) {
        throw Err(9006, "Missing image data to save PNG");
    }

    if (_bytesPerChannel > 2) {
        throw Err(9007, std::format("Cannot save {} bit", _bytesPerChannel * 8));
    }

    const double t0 = time();

    std::string f = filename.string();
    FILE* fp = fopen(f.c_str(), "wb");
    if (fp == nullptr) {
        throw Err(9008, std::format("Cannot create PNG file '{}'", filename.string()));
    }

    // initialize stuff
    png_structp png = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        nullptr,
        nullptr,
        nullptr
    );
    if (!png) {
        throw Err(9009, "Failed to create PNG struct");
    }

    // Compression levels 1-9.
    //   -1 = Default compression
    //    0 = No compression
    //    1 = Best speed
    //    9 = Best compression
    png_set_compression_level(png, -1);
    png_set_filter(png, 0, PNG_FILTER_NONE);
    png_set_compression_mem_level(png, 8);
    png_set_compression_strategy(png, Z_DEFAULT_STRATEGY);
    png_set_compression_window_bits(png, 15);
    png_set_compression_method(png, 8);
    png_set_compression_buffer_size(png, 8192);

    png_infop info = png_create_info_struct(png);
    if (!info) {
        throw Err(9010, "Failed to create PNG info struct");
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        throw Err(9011, "One of the called PNG functions failed");
    }

    png_init_io(png, fp);

    const int colorType = [](int channels) {
        switch (channels) {
            case 1: return PNG_COLOR_TYPE_GRAY;
            case 2: return PNG_COLOR_TYPE_GRAY_ALPHA;
            case 3: return PNG_COLOR_TYPE_RGB;
            case 4: return PNG_COLOR_TYPE_RGB_ALPHA;
            default: throw std::logic_error("Unhandled case label");
        }
    }(_nChannels);

    // write header
    png_set_IHDR(
        png,
        info,
        _size.x,
        _size.y,
        _bytesPerChannel * 8,
        colorType,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );

    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
        png_set_bgr(png);
    }
    png_write_info(png, info);

    // swap big-endian to little endian
    if (_bytesPerChannel == 2) {
        png_set_swap(png);
    }

    std::vector<png_bytep> rowPtrs(_size.y);
    for (int y = 0; y < _size.y; y++) {
        const size_t idx = static_cast<size_t>(_size.y) - 1 - static_cast<size_t>(y);
        rowPtrs[idx] = &_data[y * _size.x * _nChannels * _bytesPerChannel];
    }
    png_write_image(png, rowPtrs.data());
    rowPtrs.clear();

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    const double t = (time() - t0) * 1000.0;
    Log::Debug(std::format("'{}' was saved successfully ({:.2f} ms)", filename.string(), t));
}

unsigned char* Image::data() {
    return _data;
}

const unsigned char* Image::data() const {
    return _data;
}

int Image::channels() const {
    return _nChannels;
}

int Image::bytesPerChannel() const {
    return _bytesPerChannel;
}

ivec2 Image::size() const {
    return _size;
}

void Image::setSize(ivec2 size) {
    _size = std::move(size);
}

void Image::setChannels(int channels) {
    _nChannels = channels;
}

void Image::setBytesPerChannel(int bpc) {
    _bytesPerChannel = bpc;
}

void Image::allocateOrResizeData() {
    const double t0 = time();

    const unsigned int dataSize = _nChannels * _size.x * _size.y * _bytesPerChannel;
    if (dataSize == 0) {
        throw Err(
            9012,
            std::format(
                "Invalid image size {}x{} {} channels",
                _size.x, _size.y, _nChannels
            )
        );
    }

    if (_data && _dataSize != dataSize) {
        // re-allocate if needed
        delete[] _data;
        _data = nullptr;
        _dataSize = 0;
    }

    if (!_data) {
        _data = new unsigned char[dataSize];
        _dataSize = dataSize;

        Log::Debug(std::format(
            "Allocated {} bytes for image data ({:.2f} ms)",
            _dataSize, (time() - t0) * 1000.0
        ));
    }
}

} // namespace sgct
