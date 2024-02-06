/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __FFMPEG_CAPTURE_H__
#define __FFMPEG_CAPTURE_H__

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <functional>
#include <string>
#include <vector>

class Capture {
public:
    ~Capture();
    bool initialize();
    void setVideoHost(std::string hostAdress);
    void setVideoDevice(std::string videoDeviceName);
    void setVideoDecoderCallback(
        std::function<void(uint8_t ** data, int width, int height)> cb);
    void addOption(std::pair<std::string, std::string> option);
    bool poll();

    std::string videoHost() const;
    int width() const;
    int height() const;
    std::string format() const;

private:
    bool initVideoStream();
    int openCodeContext(AVFormatContext* fmt_ctx, AVMediaType type, int& streamIndex);
    bool allocateVideoDecoderData(AVPixelFormat pix_fmt);
    int decodePacket(int& gotVideoPtr);
    void cleanup();

    AVDictionary* _options = nullptr;
    AVFormatContext* _fmtContext = nullptr;
    AVStream* _videoStream = nullptr;
    AVCodecContext* _videoCodecContext = nullptr;
    SwsContext* _videoScaleContext = nullptr;
    AVFrame* _frame = nullptr; // holds src format frame
    AVFrame* _tempFrame = nullptr; // holds dst format frame

    AVPixelFormat _dstPixFmt = AV_PIX_FMT_BGR24;
    AVPacket _pkt;

    size_t _decodedVideoFrames = 0;
    bool _isInitialized = false;

    int _width = 0;
    int _height = 0;
    int _videoStreamIdx = -1;

    std::string _videoHost;
    std::string _videoDevice;
    std::string _videoDstFormat;
    std::string _videoStrFormat;
    std::vector<std::pair<std::string, std::string>> _userOptions;

    // callback function pointer
    std::function<void(uint8_t** data, int width, int height)> _videoDecoderCallback;
};

#endif // __FFMPEG_CAPTURE_H__
