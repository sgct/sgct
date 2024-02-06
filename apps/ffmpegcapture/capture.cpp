/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "capture.h"

#include <sgct/log.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <algorithm>

Capture::~Capture() {
    cleanup();
}

bool Capture::initialize() {
    if (_videoDevice.empty()) {
        sgct::Log::Error("No video device specified");
        cleanup();
        return false;
    }

    // set log level
    // av_log_set_level(AV_LOG_INFO);
    av_log_set_level(AV_LOG_QUIET);

    // register all formats and mCodecs
    av_register_all();
    avdevice_register_all();
    // avformat_network_init();

    // reduce latency
    av_dict_set(&_options, "fflags", "nobuffer", 0);
    // reduce the latency by flushing out packets immediately
    av_dict_set(&_options, "fflags", "flush_packets", 0);

    // set user options
    for (const std::pair<const std::string, std::string>& p : _userOptions) {
        av_dict_set(&_options, p.first.c_str(), p.second.c_str(), 0);
    }

    // --------------------------------------------------
    //check out https://ffmpeg.org/ffmpeg-devices.html
    // --------------------------------------------------
#ifdef WIN32
    AVInputFormat* iformat = av_find_input_format("dshow");
    const std::string inputName = "video=" + _videoDevice;
#elif defined __APPLE__
    AVInputFormat* iformat = av_find_input_format("avfoundation");
    const std::string inputName = _videoDevice;
#else  //linux NOT-Tested
    AVInputFormat* iformat = av_find_input_format("video4linux2");
    const std::string inputName = _videoDevice;
#endif

    if (avformat_open_input(&_fmtContext, inputName.c_str(), iformat, &_options) < 0) {
        sgct::Log::Error("Could not open capture input");
        cleanup();
        return false;
    }

    // retrieve stream information
    if (avformat_find_stream_info(_fmtContext, nullptr) < 0) {
        sgct::Log::Error("Could not find stream information");
        cleanup();
        return false;
    }

    if (!initVideoStream()) {
        cleanup();
        return false;
    }

    //dump format info to console
    av_dump_format(_fmtContext, 0, inputName.c_str(), 0);

    if (_videoCodecContext) {
        if (!allocateVideoDecoderData(_videoCodecContext->pix_fmt)) {
            cleanup();
            return false;
        }
    }

    // initialize packet, set data to nullptr, let the demuxer fill it
    av_init_packet(&_pkt);
    _pkt.data = nullptr;
    _pkt.size = 0;

    // success
    _isInitialized = true;
    return true;
}

std::string Capture::videoHost() const {
    return _videoHost;
}

int Capture::width() const {
    return _width;
}

int Capture::height() const {
    return _height;
}

std::string Capture::format() const {
    return _videoStrFormat;
}

void Capture::setVideoHost(std::string hostAdress) {
    _videoHost = std::move(hostAdress);
}

void Capture::setVideoDevice(std::string videoDeviceName) {
    _videoDevice = std::move(videoDeviceName);
}

void Capture::setVideoDecoderCallback(
                            std::function<void(uint8_t** data, int width, int height)> cb)
{
    _videoDecoderCallback = std::move(cb);
}

void Capture::addOption(std::pair<std::string, std::string> option) {
    _userOptions.push_back(std::move(option));
}

bool Capture::poll() {
    if (!_isInitialized) {
        return false;
    }

    bool allOk = true;

    if (av_read_frame(_fmtContext, &_pkt) >= 0) {
        AVPacket orig_pkt = _pkt;

        do {
            int gotVideoFrame;
            const int ret = decodePacket(gotVideoFrame);
            if (ret < 0) {
                sgct::Log::Error("Failed to decode package");
                allOk = false;
                break;
            }

            _pkt.data += ret;
            _pkt.size -= ret;
        } while (_pkt.size > 0);

        av_free_packet(&orig_pkt);
    }

    return allOk;
}

bool Capture::initVideoStream() {
    // open video stream
    int res = openCodeContext(_fmtContext, AVMEDIA_TYPE_VIDEO, _videoStreamIdx);
    if (res >= 0) {
        _videoStream = _fmtContext->streams[_videoStreamIdx];
        _videoCodecContext = _videoStream->codec;

        _width = _videoCodecContext->width;
        _height = _videoCodecContext->height;
    }

    return res >= 0;
}

int Capture::openCodeContext(AVFormatContext* fmt, AVMediaType type, int& streamIndex) {
    const int ret = av_find_best_stream(fmt, type, -1, -1, nullptr, 0);
    if (ret < 0) {
        sgct::Log::Error("Could not find %s stream", av_get_media_type_string(type));
        return ret;
    }

    streamIndex = ret;
    AVStream* st = fmt->streams[streamIndex];

    // find decoder for the stream
    AVCodecContext* decCxt = st->codec;
    decCxt->thread_count = 0; // auto number of threads
    decCxt->flags |= 0x00080000; // CODEC_FLAG_LOW_DELAY
    decCxt->flags2 |= 0x00000001; // CODEC_FLAG2_FAST

    AVCodec* dec = avcodec_find_decoder(decCxt->codec_id);

    if (!dec) {
        sgct::Log::Error("Could not find %s codec", av_get_media_type_string(type));
        return AVERROR(EINVAL);
    }

    // Init the decoders, with or without reference counting
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "0", 0);

    const int decodeRet = avcodec_open2(decCxt, dec, &opts);
    if (decodeRet < 0) {
        sgct::Log::Error("Could not open %s codec", av_get_media_type_string(type));
        return ret;
    }

    return 0;
}

bool Capture::allocateVideoDecoderData(AVPixelFormat pix_fmt) {
    if (pix_fmt != _dstPixFmt) {
        char buf[256];
        size_t found;

        _videoStrFormat = std::string(av_get_pix_fmt_string(buf, 256, pix_fmt));
        found = _videoStrFormat.find(' ');
        if (found != std::string::npos) {
            _videoStrFormat = _videoStrFormat.substr(0, found); // delate irrelevant data
        }

        _videoDstFormat.assign(av_get_pix_fmt_string(buf, 256, _dstPixFmt));
        found = _videoDstFormat.find(' ');
        if (found != std::string::npos) {
            _videoDstFormat = _videoDstFormat.substr(0, found); // delate irrelevant data
        }

        sgct::Log::Info(
            "Creating video scaling context (%s->%s)",
            _videoStrFormat.c_str(), _videoDstFormat.c_str()
        );

        //create context for frame convertion
        _videoScaleContext = sws_getContext(
            _width,
            _height,
            pix_fmt,
            _width,
            _height,
            _dstPixFmt,
            SWS_FAST_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );
        if (!_videoScaleContext) {
            sgct::Log::Error("Could not allocate frame conversion context");
            return false;
        }
    }

    _tempFrame = av_frame_alloc();
    if (!_tempFrame) {
        sgct::Log::Error("Could not allocate temp frame data");
        return false;
    }

    _tempFrame->width = _width;
    _tempFrame->height = _height;
    _tempFrame->format = _dstPixFmt;

    const int ret = av_image_alloc(
        _tempFrame->data,
        _tempFrame->linesize,
        _width,
        _height,
        _dstPixFmt,
        1
    );
    if (ret < 0) {
        sgct::Log::Error("Could not allocate temp frame buffer");
        return false;
    }

    if (!_frame) {
        _frame = av_frame_alloc();
    }

    if (!_frame) {
        sgct::Log::Error("Could not allocate frame data");
        return false;
    }

    return true;
}

int Capture::decodePacket(int& gotVideo) {
    gotVideo = 0;

    if (_pkt.stream_index != _videoStreamIdx || !_videoCodecContext) {
        return _pkt.size;
    }

    // decode video frame
    int ret = avcodec_decode_video2(_videoCodecContext, _frame, &gotVideo, &_pkt);

    if (ret < 0) {
        char Buffer[AV_ERROR_MAX_STRING_SIZE];
        char* err = av_make_error_string(Buffer, AV_ERROR_MAX_STRING_SIZE, ret);
        sgct::Log::Error("Video decoding error: %s", err);
        return ret;
    }

    const int decoded = std::min(ret, _pkt.size);
    if (gotVideo != 0) {
        if (_videoCodecContext->pix_fmt != _dstPixFmt) {
            // convert to destination pixel format
            const int scaleRet = sws_scale(
                _videoScaleContext,
                _frame->data,
                _frame->linesize,
                0,
                _height,
                _tempFrame->data,
                _tempFrame->linesize
            );
            if (scaleRet < 0) {
                sgct::Log::Error(
                    "Failed to convert decoded frame to %s", _videoDstFormat.c_str()
                );
                return scaleRet;
            }
        }
        else {
            const int copyRet = av_frame_copy(_tempFrame, _frame);
            if (copyRet < 0) {
                sgct::Log::Error("Failed to copy frame!\n");
                return copyRet;
            }
        }

        // store _tempFrame;
        if (_videoDecoderCallback) {
            _videoDecoderCallback(_tempFrame->data, _width, _height);
        }

        _decodedVideoFrames++;
    }
    return decoded;
}

void Capture::cleanup() {
    _isInitialized = false;
    _videoDecoderCallback = nullptr;

    if (_videoCodecContext) {
        avcodec_close(_videoCodecContext);
        _videoCodecContext = nullptr;
    }

    if (_fmtContext) {
        avformat_close_input(&_fmtContext);
        _fmtContext = nullptr;
    }

    if (_frame) {
        av_frame_free(&_frame);
        _frame = nullptr;
    }

    if (_tempFrame) {
        av_frame_free(&_tempFrame);
        _tempFrame = nullptr;
    }

    if (_videoScaleContext) {
        sws_freeContext(_videoScaleContext);
        _videoScaleContext = nullptr;
    }

    _videoStream = nullptr;

    _width = 0;
    _height = 0;
}
