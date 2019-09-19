/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

// png.h needs all of this included before its own include.. sigh
#include <stdio.h>
#include <fstream>
#include <algorithm>

#include <png.h>
#include <pngpriv.h>

#ifdef SGCT_HAS_TURBOJPEG
#include <jpeglib.h>
#include <turbojpeg.h>
#endif // SGCT_HAS_TURBOJPEG

#include <sgct/image.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>

namespace {
    constexpr const int PngBytesToCheck = 8;
    constexpr const int TgaBytesToCheck = 18;

    struct PNG_IO_DATA {
        size_t memOffset;
        unsigned char* data;
    };

#ifdef SGCT_HAS_TURBOJPEG
    //---------------- JPEG helpers -----------------
    struct my_error_mgr {
        struct jpeg_error_mgr pub; // "public" fields
        jmp_buf setjmp_buffer; // for return to caller
    };

    // Here's the routine that will replace the standard error_exit method:
    METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
        // cinfo->err really points to a my_error_mgr struct, so coerce pointer
        my_error_mgr* myerr = reinterpret_cast<my_error_mgr*>(cinfo->err);

        // Always display the message.
        // We could postpone this until after returning, if we chose.
        (*cinfo->err->output_message)(cinfo);

        // Return control to the setjmp point
        longjmp(myerr->setjmp_buffer, 1);
    }

#endif // SGCT_HAS_TURBOJPEG

    void readPNGFromBuffer(png_structp png_ptr, png_bytep outData, png_size_t length) {
        if (length <= 0) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image: PNG reading error. Invalid length"
            );
            return;
        }

        // The file 'handle', a pointer, is stored in png_ptr->io_ptr
        if (png_ptr->io_ptr == nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image: PNG reading error. Invalid source pointer"
            );
            return;
        }

        if (outData == nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image: PNG reading error. Invalid destination pointer"
            );
            return;
        }

        // copy buffer
        PNG_IO_DATA* ioPtr = reinterpret_cast<PNG_IO_DATA*>(png_ptr->io_ptr);
        memcpy(outData, ioPtr->data + ioPtr->memOffset, length);
        ioPtr->memOffset += length;
    }


    sgct_core::Image::FormatType getFormatType(std::string filename) {
        std::transform(
            filename.begin(),
            filename.end(),
            filename.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

        if (filename.find(".png") != std::string::npos) {
            return sgct_core::Image::FormatType::PNG;
        }
        if (filename.find(".jpg") != std::string::npos) {
            return sgct_core::Image::FormatType::JPEG;
        }
        if (filename.find(".jpeg") != std::string::npos) {
            return sgct_core::Image::FormatType::JPEG;
        }
        if (filename.find(".tga") != std::string::npos) {
            return sgct_core::Image::FormatType::TGA;
        }
        return sgct_core::Image::FormatType::Unknown;
    }

} // namespace

namespace sgct_core {

Image::~Image() {
    cleanup();
}

bool Image::load(std::string filename) {
    if (filename.empty()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Cannot load emtpy filepath\n"
        );
        return false;
    }

    bool res = false;
    const double t0 = sgct::Engine::getTime();

    switch (getFormatType(filename)) {
        case FormatType::PNG:
            res = loadPNG(filename);
            if (res) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "Image: '%s' was loaded successfully (%.2f ms)\n",
                    filename.c_str(), (sgct::Engine::getTime() - t0) * 1000.0
                );
            }
            break;

        case FormatType::JPEG:
            res = loadJPEG(filename);
            if (res) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "Image: '%s' was loaded successfully (%.2f ms)\n",
                    filename.c_str(), (sgct::Engine::getTime() - t0) * 1000.0
                );
            }
            break;

        case FormatType::TGA:
            res = loadTGA(filename);
            if (res) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "Image: '%s' was loaded successfully (%.2f ms)\n",
                    filename.c_str(), (sgct::Engine::getTime() - t0) * 1000.0
                );
            }
            break;

        default:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: Unknown file '%s'\n", filename.c_str()
            );
            break;
    }

    return res;
}

bool Image::loadJPEG(std::string filename) {
#ifdef SGCT_HAS_TURBOJPEG
    if (filename.empty()) {
        // one char + dot and suffix and is 5 char
        return false;
    }

    mFilename = std::move(filename);
    
    my_error_mgr jerr;
    jpeg_decompress_struct cinfo;
    FILE* fp = nullptr;
    JSAMPARRAY buffer;
    std::size_t row_stride;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if (fopen_s(&fp, mFilename.c_str(), "rbS") != 0 || !fp) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open JPEG texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#else
    fp = fopen(mFilename.c_str(), "rb");
    if (fp == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open JPEG texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#endif

    // Step 1: allocate and initialize JPEG decompression object

    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer)) {
        // If we get here, the JPEG code has signaled an error.
        // We need to clean up the JPEG object, close the input file, and return.
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open JPEG texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
    
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    row_stride = cinfo.output_width * cinfo.output_components;

    // SGCT uses BGR so convert to that
    cinfo.out_color_space = mPreferBGRForImport ? JCS_EXT_BGR : JCS_EXT_RGB;

    // only support 8-bit per color depth for jpeg even if the format supports up to
    // 12-bit
    mBytesPerChannel = 1; 
    mChannels = cinfo.output_components;
    mSizeX = cinfo.output_width;
    mSizeY = cinfo.output_height;
    
    if (!allocateOrResizeData()) {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return false;
    }

    // Make a one-row-high sample array that will go away when done with image
    buffer = (*cinfo.mem->alloc_sarray)(
        reinterpret_cast<j_common_ptr>(&cinfo),
        JPOOL_IMAGE,
        static_cast<JDIMENSION>(row_stride),
        1
    );

    size_t r = mSizeY - 1;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        // flip vertically
        memcpy(mData + row_stride*r, buffer[0], row_stride);
        r--;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "Image: Loaded %s (%dx%d).\n", mFilename.c_str(), mSizeX, mSizeY
    );

    return true;
#else
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Error,
        "SGCT was compiled without support for TurbJPEG, which prevents loading of %s",
        mFilename.c_str()
    );
    return false;
#endif // SGCT_HAS_TURBOJPEG
}

bool Image::loadJPEG(unsigned char* data, size_t len) {
#ifdef SGCT_HAS_TURBOJPEG
    if (data == nullptr || len <= 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image: failed to load JPEG from memory. Invalid input data"
        );
        return false;
    }
    
    tjhandle turbo_jpeg_handle = tjInitDecompress();

    // only supports 8-bit per color depth for jpeg even if the format supports 12-bit
    mBytesPerChannel = 1; 
    
    int jpegsubsamp;
    int colorspace;
    int decompress = tjDecompressHeader3(
        turbo_jpeg_handle,
        data,
        static_cast<unsigned long>(len),
        reinterpret_cast<int*>(&mSizeX),
        reinterpret_cast<int*>(&mSizeY),
        &jpegsubsamp,
        &colorspace
    );
    if (decompress < 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image: failed to load JPEG from memory. Error: %s\n", tjGetErrorStr()
        );
        tjDestroy(turbo_jpeg_handle);
        return false;
    }
    
    int pixelformat = 0;
    switch (jpegsubsamp) {
        case TJSAMP_444:
        case TJSAMP_422:
        case TJSAMP_420:
        case TJSAMP_440:
            mChannels = 3;
            pixelformat = mPreferBGRForImport ? TJPF_BGR : TJPF_RGB;
            break;
        case TJSAMP_GRAY:
            mChannels = 1;
            pixelformat = TJPF_GRAY;
            break;
        default:
            mChannels = static_cast<size_t>(-1);
            break;
    }
    
    if (mChannels < 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image: failed to load JPEG from memory. Unsupported chrominance "
            "subsampling\n"
        );
        tjDestroy(turbo_jpeg_handle);
        return false;
    }
    
    if (!allocateOrResizeData()) {
        tjDestroy(turbo_jpeg_handle);
        return false;
    }

    if (!mData) {
        tjDestroy(turbo_jpeg_handle);
        return false;

    }
    
    decompress = tjDecompress2(
        turbo_jpeg_handle,
        data,
        static_cast<unsigned long>(len),
        mData,
        static_cast<int>(mSizeX),
        0,
        static_cast<int>(mSizeY),
        pixelformat,
        TJFLAG_FASTDCT | TJFLAG_BOTTOMUP
    );
    if (decompress < 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image: failed to load JPEG from memory. Error: %s\n", tjGetErrorStr()
        );
        tjDestroy(turbo_jpeg_handle);
        
        delete[] mData;
        mData = nullptr;
        
        return false;
    }
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Error,
        "Image: Loaded %dx%d JPEG from memory\n", mSizeX, mSizeY
    );
    
    tjDestroy(turbo_jpeg_handle);
    return true;
#else
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Error,
        "SGCT was compiled without support for TurbJPEG, which prevents loading of %s",
        mFilename.c_str()
    );
    return false;
#endif //SGCT_HAS_TURBOJPEG
}

bool Image::loadPNG(std::string filename) {
    if (filename.empty()) {
        // one char + dot and suffix and is 5 char
        return false;
    }

    mFilename = std::move(filename);

    png_structp png_ptr;
    png_infop info_ptr;
    unsigned char header[PngBytesToCheck];
    int color_type, bpp;

    FILE* fp = nullptr;
    #if (_MSC_VER >= 1400)
    if (fopen_s(&fp, mFilename.c_str(), "rbS") != 0 || !fp) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open PNG texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
    #else
    fp = fopen(mFilename.c_str(), "rb");
    if (fp == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open PNG texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
    #endif

    size_t result = fread(header, 1, PngBytesToCheck, fp);
    if (result != PngBytesToCheck ||
        png_sig_cmp(reinterpret_cast<png_byte*>(&header[0]), 0, PngBytesToCheck))
    {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: file '%s' is not in PNG format\n", mFilename.c_str()
        );
        fclose(fp);
        return false;
    }
    ///
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't initialize PNG file for reading: %s\n", mFilename.c_str()
        );
        fclose(fp);
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't allocate memory to read PNG file: %s\n", mFilename.c_str()
        );
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        fclose(fp);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Exception occurred while reading PNG file: %s\n",
            mFilename.c_str()
        );
        return false;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, PngBytesToCheck);
    png_read_info(png_ptr, info_ptr);
    
    png_get_IHDR(
        png_ptr,
        info_ptr,
        reinterpret_cast<png_uint_32*>(&mSizeX),
        reinterpret_cast<png_uint_32*>(&mSizeY),
        &bpp,
        &color_type,
        nullptr,
        nullptr,
        nullptr
    );
    
    //set options
    if (mPreferBGRForImport) {
        png_set_bgr(png_ptr);
    }
    if (bpp < 8) {
        png_set_packing(png_ptr);
    }
    else if (bpp == 16) {
        // Load 16-bit as 8-bit
        png_set_strip_16(png_ptr);
        bpp = 8;
    }

    mBytesPerChannel = bpp / 8;

    if (color_type == PNG_COLOR_TYPE_GRAY) {
        mChannels = 1;
        if (bpp < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
            png_read_update_info(png_ptr, info_ptr);
        }
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        mChannels = 2;
    }
    else if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_expand(png_ptr); // expand to RGB -> PNG_TRANSFORM_EXPAND
        mChannels = 3;
    }
    else if (color_type == PNG_COLOR_TYPE_RGB) {
        mChannels = 3;
    }
    else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        mChannels = 4;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Unsupported format '%s'\n", mFilename.c_str()
        );
        fclose(fp);
        return false;
    }

    if (!allocateOrResizeData()) {
        fclose(fp);
        return false;
    }

    //flip the image
    size_t pos = mDataSize;
    for (size_t i = 0; i < mSizeY; i++) {
        pos -= mSizeX * mChannels;
        png_read_row(png_ptr, &mData[pos], nullptr);
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    fclose(fp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "Image: Loaded %s (%dx%d %d-bit).\n",
        mFilename.c_str(), mSizeX, mSizeY, mBytesPerChannel * 8
    );

    return true;
}

bool Image::loadPNG(unsigned char* data, size_t len) {
    if (data == nullptr || len <= PngBytesToCheck) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image: failed to load PNG from memory. Invalid input data"
        );
        return false;
    }
    
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned char header[PngBytesToCheck];
    int color_type, bpp;
    
    //get header
    memcpy(header, data, PngBytesToCheck);
    if (!png_check_sig( header, PngBytesToCheck)) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Invalid PNG file header\n"
        );
        return false;
    }
    
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't initialize PNG\n"
        );
        return false;
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't allocate memory to read PNG data\n"
        );
        return false;
    }
    
    // set the read position in memory
    PNG_IO_DATA io;
    io.memOffset = PngBytesToCheck;
    io.data = data;
    png_set_read_fn(png_ptr, &io, readPNGFromBuffer);
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Exception occurred while reading PNG data\n"
        );
        return false;
    }
    
    png_set_sig_bytes(png_ptr, PngBytesToCheck);
    png_read_info(png_ptr, info_ptr);
    
    png_get_IHDR(
        png_ptr,
        info_ptr,
        reinterpret_cast<png_uint_32*>(&mSizeX),
        reinterpret_cast<png_uint_32*>(&mSizeY),
        &bpp,
        &color_type,
        nullptr,
        nullptr,
        nullptr
    );
    
    // set options
    if (mPreferBGRForImport) {
        png_set_bgr(png_ptr);
    }
    if (bpp < 8) {
        png_set_packing(png_ptr);
    }
    else if (bpp == 16) {
        png_set_swap(png_ptr); // PNG_TRANSFORM_SWAP_ENDIAN
    }

    mBytesPerChannel = bpp / 8;

    if (color_type == PNG_COLOR_TYPE_GRAY) {
        mChannels = 1;
        if (bpp < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
            png_read_update_info(png_ptr, info_ptr);
        }
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        mChannels = 2;
    }
    else if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_expand(png_ptr); // expand to RGB -> PNG_TRANSFORM_EXPAND
        mChannels = 3;
    }
    else if (color_type == PNG_COLOR_TYPE_RGB) {
        mChannels = 3;
    }
    else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        mChannels = 4;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Unsupported format '%s'\n", mFilename.c_str()
        );
        return false;
    }

    if (!allocateOrResizeData()) {
        return false;
    }

    // flip the image
    size_t pos = mDataSize;
    for (size_t i = 0; i < mSizeY; i++) {
        pos -= mSizeX * mChannels;
        png_read_row(png_ptr, &mData[pos], nullptr);
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "Image: Loaded %dx%d %d-bit PNG from memory\n",
        mSizeX, mSizeY, mBytesPerChannel * 8
    );
    
    return true;
}

bool Image::loadTGA(std::string filename) {
    if (filename.empty()) {
        // one char + dot and suffix and is 5 char
        return false;
    }

    mFilename = std::move(filename);

    unsigned char header[TgaBytesToCheck];

    FILE* fp = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&fp, mFilename.c_str(), "rbS") != 0 || !fp) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open TGA texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#else
    fp = fopen(mFilename.c_str(), "rb");
    if (fp == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't open TGA texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#endif

    size_t result = fread(header, 1, TgaBytesToCheck, fp);
    if (result != TgaBytesToCheck) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: file '%s' is not in TGA format\n", mFilename.c_str()
        );
        fclose(fp);
        return false;
    }

    unsigned char data_type = header[2];
    mSizeX = static_cast<int>(header[12]) + (static_cast<int>(header[13]) << 8);
    mSizeY = static_cast<int>(header[14]) + (static_cast<int>(header[15]) << 8);
    mChannels = static_cast<int>(header[16]) / 8;

    if (!allocateOrResizeData()) {
        fclose(fp);
        return false;
    }
    
    if (data_type == 10) {
        // RGB rle
        if (!decodeTGARLE(fp)) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: file '%s' is corrupted\n", mFilename.c_str()
            );
            fclose(fp);
            return false;
        }
    }
    else {
        result = fread(mData, 1, mDataSize, fp);

        if (result != mDataSize) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: file '%s' is corrupted\n", mFilename.c_str()
            );
            fclose(fp);
            return false;
        }
    }

    // done with the file
    fclose(fp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "Image: Loaded %s (%dx%d %d-bit).\n",
        mFilename.c_str(), mSizeX, mSizeY, mBytesPerChannel * 8
    );
    return true;
}

bool Image::loadTGA(unsigned char* data, size_t len) {
    if (data == nullptr || len <= TgaBytesToCheck) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image: failed to load TGA from memory. Invalid input data"
        );
        return false;
    }

    unsigned char data_type = data[2];
    mSizeX = static_cast<int>(data[12]) + (static_cast<int>(data[13]) << 8);
    mSizeY = static_cast<int>(data[14]) + (static_cast<int>(data[15]) << 8);
    mChannels = static_cast<int>(data[16]) / 8;

    if (!allocateOrResizeData()) {
        return false;
    }

    if (data_type == 10) {
        //RGB rle
        if (!decodeTGARLE(&data[TgaBytesToCheck], len - TgaBytesToCheck)) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: data is corrupted or insufficent\n"
            );
            return false;
        }
    }
    else {
        if (len < (mDataSize + TgaBytesToCheck)) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: data is corrupted or insufficent\n"
            );
            return false;
        }
        
        memcpy(mData, &data[TgaBytesToCheck], mDataSize);
    }
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "Image: Loaded %dx%d TGA from memory\n", mSizeX, mSizeY
    );

    return true;
}

bool Image::decodeTGARLE(FILE* fp) {
    const size_t pixelCount = mSizeX * mSizeY;
    size_t currentPixel = 0;
    size_t currentByte = 0;

    do {
        unsigned char chunkheader = 0;

        if (fread(&chunkheader, 1, 1, fp) == 0) {
            return false;
        }

        if (chunkheader < 128) {
            chunkheader++;
            for (unsigned char counter = 0; counter < chunkheader; counter++) {
                const size_t res = fread(&mData[currentByte], 1, mChannels, fp);
                if (res != mChannels) {
                    return false;
                }

                currentByte += mChannels;
                currentPixel++;

                if (currentPixel > pixelCount) {
                    break;
                }
            }
        }
        else {
            chunkheader -= 127;
            const size_t res = fread(&mData[currentByte], 1, mChannels, fp);
            if (res != mChannels) {
                return false;
            }

            unsigned char* chunkPtr = &mData[currentByte];
            currentByte += mChannels;
            currentPixel++;

            for (short counter = 1; counter < chunkheader; counter++) {
                memcpy(&mData[currentByte], chunkPtr, mChannels);

                currentByte += mChannels;
                currentPixel++;

                if (currentPixel > pixelCount) {
                    break;
                }
            }
        }
    } while (currentPixel < pixelCount);

    return true;
}

bool Image::decodeTGARLE(unsigned char* data, size_t len) {
    size_t pixelcount = mSizeX * mSizeY;
    size_t currentpixel = 0;
    size_t currentbyte = 0;
    unsigned char chunkheader;
    unsigned char* chunkPtr;

    size_t index = 0;

    do {
        chunkheader = 0;

        if (len > index) {
            chunkheader = data[index];
            index++;
        }
        else {
            return false;
        }

        if (chunkheader < 128) {
            chunkheader++;
            
            for (unsigned char counter = 0; counter < chunkheader; counter++) {
                if (len >= (index + mChannels)) {
                    memcpy(&mData[currentbyte], &data[index], mChannels);
                    index += mChannels;
                    
                    currentbyte += mChannels;
                    currentpixel++;
                }
                else {
                    return false;
                }

                if (currentpixel > pixelcount) {
                    return false;
                }
            }
        }
        else {
            chunkheader -= 127;
            if (len >= (index + mChannels)) {
                memcpy(&mData[currentbyte], &data[index], mChannels);
                index += mChannels;
                
                chunkPtr = &mData[currentbyte];
                currentbyte += mChannels;
                currentpixel++;
            }
            else {
                return false;
            }

            for (short counter = 1; counter < chunkheader; counter++) {
                memcpy(&mData[currentbyte], chunkPtr, mChannels);

                currentbyte += mChannels;
                currentpixel++;

                if (currentpixel > pixelcount) {
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);

    return true;
}

bool Image::save() {
    if (mFilename.empty()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Filename not set for saving image\n"
        );
        return false;
    }

    switch (getFormatType(mFilename)) {
        case FormatType::PNG:
            savePNG();
            return true;
        case FormatType::JPEG:
            saveJPEG();
            return true;
        case FormatType::TGA:
            saveTGA();
            return true;
        default:
            // not found
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: Cannot save file '%s'\n", mFilename.c_str()
            );
            return false;
    }
}

bool Image::savePNG(std::string filename, int compressionLevel) {
    setFilename(std::move(filename));
    return savePNG(compressionLevel);
}

bool Image::savePNG(int compressionLevel) {
    if (mData == nullptr) {
        return false;
    }

    if (mBytesPerChannel > 2) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Cannot save %d-bit PNG\n", mBytesPerChannel * 8
        );
        return false;
    }

    double t0 = sgct::Engine::getTime();
    
    FILE* fp = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s( &fp, mFilename.c_str(), "wb") != 0 || !fp) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create PNG file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#else
    fp = fopen(mFilename.c_str(), "wb");
    if (fp == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create PNG file '%s'\n", mFilename.c_str()
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
    // png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
    
    png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
    
    png_set_compression_mem_level(png_ptr, 8);
    // png_set_compression_mem_level(png_ptr, MAX_MEM_LEVEL);
    // png_set_compression_strategy(png_ptr, Z_HUFFMAN_ONLY);
    
    sgct::Settings::instance()->getUseRLE() ? 
        png_set_compression_strategy(png_ptr, Z_RLE) :
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

    int color_type = -1;
    switch (mChannels) {
        case 1:
            color_type = PNG_COLOR_TYPE_GRAY;
            break;
        case 2:
            color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
            break;
        case 3:
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case 4:
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
            break;
    }

    if (color_type == -1) {
        return false;
    }

    // write header
    png_set_IHDR(
        png_ptr,
        info_ptr,
        static_cast<int>(mSizeX),
        static_cast<int>(mSizeY),
        static_cast<int>(mBytesPerChannel) * 8,
        color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );
    
    if (mPreferBGRForExport &&
        (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA))
    {
        png_set_bgr(png_ptr);
    }
    png_write_info(png_ptr, info_ptr);

    // write bytes
    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    // swap big-endian to little endian
    if (mBytesPerChannel == 2) {
        png_set_swap(png_ptr);
    }

    for (size_t y = 0; y < mSizeY; y++) {
        mRowPtrs[(mSizeY - 1) - y] = reinterpret_cast<png_bytep>(
            &mData[y * mSizeX * mChannels * mBytesPerChannel]
        );
    }
    png_write_image(png_ptr, mRowPtrs);

    // end write
    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Image: '%s' was saved successfully (%.2f ms)\n",
        mFilename.c_str(),
        (sgct::Engine::getTime() - t0) * 1000.0
    );

    return true;
}

bool Image::saveJPEG(int quality) {
#ifdef SGCT_HAS_TURBOJPEG
    if (mData == nullptr) {
        return false;
    }

    if (mBytesPerChannel > 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Cannot save %d-bit JPEG\n", mBytesPerChannel * 8
        );
        return false;
    }

    double t0 = sgct::Engine::getTime();

    FILE* fp = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&fp, mFilename.c_str(), "wb") != 0 || !fp) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create JPEG file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#else
    fp = fopen(mFilename.c_str(), "wb");
    if (fp == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create JPEG file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#endif
    
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    
    JSAMPROW row_pointer[1]; // pointer to JSAMPLE row[s]
    size_t row_stride;       // physical row width in image buffer

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = static_cast<JDIMENSION>(mSizeX);
    cinfo.image_height = static_cast<JDIMENSION>(mSizeY);
    cinfo.input_components = static_cast<int>(mChannels);

    switch (mChannels) {
        case 4:
            cinfo.in_color_space = mPreferBGRForExport ? JCS_EXT_BGRA : JCS_EXT_RGBA;
            break;
        case 3:
        default:
            cinfo.in_color_space = mPreferBGRForExport ? JCS_EXT_BGR : JCS_RGB;
            break;
        case 2:
            cinfo.in_color_space = JCS_UNKNOWN;
            break;
        case 1:
            cinfo.in_color_space = JCS_GRAYSCALE;
            break;
    }

    if (cinfo.in_color_space == JCS_UNKNOWN) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: JPEG doesn't support two channel output\n"
        );
        return false;
    }

    jpeg_set_defaults(&cinfo);
    // limit to baseline-JPEG values
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    row_stride = mSizeX * mChannels; // JSAMPLEs per row in image_buffer

    while (cinfo.next_scanline < cinfo.image_height) {
        // flip vertically
        row_pointer[0] = &mData[(mSizeY - cinfo.next_scanline - 1) * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(fp);

    jpeg_destroy_compress(&cinfo);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Image: '%s' was saved successfully (%.2f ms)\n",
        mFilename.c_str(),
        (sgct::Engine::getTime() - t0) * 1000.0
    );
    return true;
#else
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Error,
        "SGCT was compiled without support for TurbJPEG, which prevents saving of %s",
        mFilename.c_str()
    );
    return false;
#endif
}

bool Image::saveTGA() {
    if (mData == nullptr) {
        return false;
    }

    if (mBytesPerChannel > 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Cannot save %d-bit TGA\n", mBytesPerChannel * 8
        );
        return false;
    }
    
    double t0 = sgct::Engine::getTime();

    FILE* fp = nullptr;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if (fopen_s(&fp, mFilename.c_str(), "wb") != 0 || !fp) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create TGA texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#else
    fp = fopen(mFilename.c_str(), "wb");
    if (fp == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create TGA texture file '%s'\n", mFilename.c_str()
        );
        return false;
    }
#endif

    if (mChannels == 2) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Can't create TGA texture file '%s'.\n"
            "Luminance alpha not supported by the TGA format\n",
            mFilename.c_str()
        );
        return false;
    }

    /*
     * TGA data type field
     * 0  -  No image data included
     * 1  -  Uncompressed, color-mapped images
     * 2  -  Uncompressed, RGB images
     * 3  -  Uncompressed, black and white images
     * 9  -  Runlength encoded color-mapped images
     * 10 -  Runlength encoded RGB images
     * 11 -  Compressed, black and white images
     * 32 -  Compressed color-mapped data, using Huffman, Delta, and runlength encoding
     * 33 -  Compressed color-mapped data, using Huffman, Delta, and runlength encoding
     *       4-pass quadtree-type process
     */

    unsigned char data_type;
    switch (mChannels) {
        default:
            data_type = sgct::Settings::instance()->getUseRLE() ? 10 : 2;
            //data_type = 2;//uncompressed RGB
            //data_type = 10;//RLE compressed RGB
            break;
        case 1:
            data_type = 3; //bw
            break;
    }

    // The image header
    unsigned char header[TgaBytesToCheck] = { 0 };
    header[ 2] = data_type; //datatype
    header[12] =  mSizeX        & 0xFF;
    header[13] = (mSizeX  >> 8) & 0xFF;
    header[14] =  mSizeY        & 0xFF;
    header[15] = (mSizeY >> 8)  & 0xFF;
    header[16] = static_cast<unsigned char>(mChannels * 8);  // bits per pixel

    fwrite(header, sizeof(unsigned char), sizeof(header), fp);

    // The file footer. This part is totally optional.
    static const char footer[26] =
        "\0\0\0\0"  // no extension area
        "\0\0\0\0"  // no developer directory
        "TRUEVISION-XFILE"  // yep, this is a TGA file
        ".";

    // convert the image data from RGB(a) to BGR(A)
    if (!mPreferBGRForExport) {
        mPreferBGRForImport = true; //reset BGR flag for texture manager
        
        unsigned char tmp;
        if (mChannels >= 3) {
            for (size_t i = 0; i < mDataSize; i += mChannels) {
                tmp = mData[i];
                mData[i] = mData[i + 2];
                mData[i + 2] = tmp;
            }
        }
    }

    // write row-by-row
    if (data_type != 10) {
        // Non RLE compression
        fwrite(mData, 1, mDataSize, fp);
    }
    else {
        // RLE ->only for RBG and minimum size is 3x3
        for (size_t y = 0; y < mSizeY; y++) {
            size_t pos = 0;
            while (pos < mSizeY) {
                unsigned char* row = &mData[y * mSizeX * mChannels];
                bool rle = isTGAPackageRLE(row, pos);
                size_t len = getTGAPackageLength(row, pos, rle);
                
                unsigned char packetHeader = static_cast<unsigned char>(len) - 1;
                
                if (rle) {
                    packetHeader |= (1 << 7);
                }
                
                fwrite(&packetHeader, 1, 1, fp);
                fwrite(row + pos * mChannels, mChannels, rle ? 1 : len, fp);
                
                pos += len;
            }
        }
    }

    fwrite(footer, sizeof(char), sizeof(footer), fp);

    fclose(fp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Image: '%s' was saved successfully (%.2f ms)\n",
        mFilename.c_str(), (sgct::Engine::getTime() - t0) * 1000.0
    );

    return true;
}

bool Image::isTGAPackageRLE(unsigned char* row, size_t pos) {
    if (pos == mSizeX - 1) {
        return false;
    }
    
    unsigned char* p0 = row + pos * mChannels;
    unsigned char* p1 = p0 + mChannels;
    
    // minimum three same pixels in row
    return ((pos < mSizeX - 2) &&
            memcmp(p0, p1, mChannels) == 0 &&
            memcmp(p1, p1 + mChannels, mChannels) == 0);
}

size_t Image::getTGAPackageLength(unsigned char* row, size_t pos, bool rle) {
    if (mSizeX - pos < 3) {
        return mSizeX - pos;
    }
    
    int len = 2;
    if (rle) {
        while (pos + len < mSizeX) {
            unsigned char* m1 = &row[pos * mChannels];
            unsigned char* m2 = &row[(pos + len) * mChannels];
            if (memcmp(m1, m2, mChannels) == 0) {
                len++;
            }
            else {
                return len;
            }
            
            if (len == 128) {
                return 128;
            }
        }
    }
    else {
        while (pos + len < mSizeX) {
            if (isTGAPackageRLE(row, pos + len)) {
                return len;
            }
            else {
                len++;
            }
            
            if (len == 128) {
                return 128;
            }
        }
    }
    return len;
}

void Image::setFilename(std::string filename) {
    if (filename.empty() || filename.length() < 5) {
        // one char + dot and suffix and is 5 char
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Invalid filename\n"
        );
        return;
    }

    mFilename = std::move(filename);
}

void Image::setPreferBGRExport(bool state) {
    mPreferBGRForExport = state;
}

void Image::setPreferBGRImport(bool state) {
    mPreferBGRForImport = state;
}

bool Image::getPreferBGRExport() const {
    return mPreferBGRForExport;
}

bool Image::getPreferBGRImport() const {
    return mPreferBGRForImport;
}

void Image::cleanup() {
    if (!mExternalData && mData) {
        delete[] mData;
        mData = nullptr;
        mDataSize = 0;
    }

    if (mRowPtrs) {
        delete[] mRowPtrs;
        mRowPtrs = nullptr;
    }
}

unsigned char* Image::getData() {
    return mData;
}

const unsigned char* Image::getData() const {
    return mData;
}

unsigned char* Image::getDataAt(size_t x, size_t y) {
    return &mData[(y * mSizeX + x) * mChannels];
}

size_t Image::getChannels() const {
    return mChannels;
}

size_t Image::getWidth() const {
    return mSizeX;
}

size_t Image::getHeight() const {
    return mSizeY;
}

size_t Image::getDataSize() const {
    return mDataSize;
}

size_t Image::getBytesPerChannel() const {
    return mBytesPerChannel;
}

unsigned char* Image::getSampleAt(size_t x, size_t y) {
    return &mData[(y * mSizeX + x) * mChannels * mBytesPerChannel];
}

void Image::setSampleAt(unsigned char* val, size_t x, size_t y) {
    memcpy(
        &mData[(y * mSizeX + x) * mChannels * mBytesPerChannel],
        val,
        mChannels * mBytesPerChannel
    );
}

unsigned char Image::getSampleAt(size_t x, size_t y, ChannelType c) {
    return mData[(y * mSizeX + x) * mChannels + c];
}

void Image::setSampleAt(unsigned char val, size_t x, size_t y, ChannelType c) {
    mData[(y * mSizeX + x) * mChannels + c] = val;
}

float Image::getInterpolatedSampleAt(float x, float y, ChannelType c) {
    const int px = static_cast<int>(x); // floor x
    const int py = static_cast<int>(y); // floor y
    
    // Calculate the weights for each pixel
    const float fx = x - static_cast<float>(px);
    const float fy = y - static_cast<float>(py);
    
    // if no need for interpolation
    if (fx == 0.f && fy == 0.f) {
        return static_cast<float>(getSampleAt(px, py, c));
    }
    
    const float fx1 = 1.f - fx;
    const float fy1 = 1.f - fy;
    
    const float w0 = fx1 * fy1;
    const float w1 = fx  * fy1;
    const float w2 = fx1 * fy;
    const float w3 = fx  * fy;
    
    const float p0 = static_cast<float>(getSampleAt(px, py, c));
    const float p1 = static_cast<float>(getSampleAt(px, py + 1, c));
    const float p2 = static_cast<float>(getSampleAt(px + 1, py, c));
    const float p3 = static_cast<float>(getSampleAt(px + 1, py + 1, c));
    
    return p0 * w0 + p1 * w1 + p2 * w2 + p3 * w3;
}

void Image::setDataPtr(unsigned char* dPtr) {
    if (!mExternalData && mData) {
        delete[] mData;
        mData = nullptr;
        mDataSize = 0;
    }

    allocateRowPtrs();
    
    mData = dPtr;
    mExternalData = true;
}

void Image::setSize(size_t width, size_t height) {
    mSizeX = width;
    mSizeY = height;
}

void Image::setChannels(size_t channels) {
    mChannels = channels;
}

void Image::setBytesPerChannel(size_t bpc) {
    mBytesPerChannel = bpc;
}

const std::string& Image::getFilename() const {
    return mFilename;
}

bool Image::allocateOrResizeData() {
    double t0 = sgct::Engine::getTime();
    
    size_t dataSize = mChannels * mSizeX * mSizeY * mBytesPerChannel;

    if (dataSize <= 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Invalid image size %dx%d %d channels\n",
            mSizeX, mSizeY, mChannels
        );
        return false;
    }

    if (mData && mDataSize != dataSize) {
        // re-allocate if needed
        cleanup();
    }

    if (!mData) {
        try {
            mData = new unsigned char[dataSize];
            mDataSize = dataSize;
            mExternalData = false;
        }
        catch (std::bad_alloc& ba) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Image error: Failed to allocate %d bytes of image data (%s)\n",
                dataSize, ba.what()
            );
            mData = nullptr;
            mDataSize = 0;
            return false;
        }

        if (!allocateRowPtrs()) {
            return false;
        }

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Image: Allocated %d bytes for image data (%.2f ms)\n",
            mDataSize, (sgct::Engine::getTime() - t0) * 1000.0
        );
    }

    return true;
}

bool Image::allocateRowPtrs() {
    if (mRowPtrs) {
        delete[] mRowPtrs;
        mRowPtrs = nullptr;
    }

    try {
        mRowPtrs = new png_bytep[mSizeY];
    }
    catch (std::bad_alloc& ba) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Image error: Failed to allocate pointers for image data (%s)\n",
            ba.what()
        );
        mRowPtrs = nullptr;
        return false;
    }

    return true;
}

} // namespace sgct_core
