#include "capture.h"
#include <sgct.h>

extern "C" {
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
} // extern "C"

#define USE_REF_COUNTER 1
// if any known visual studio platform
#if (_MSC_VER >= 1100) // visual studio 5 or later
#undef av_err2str
#define av_err2str(errnum) \
    av_make_error_string(reinterpret_cast<char*>(_alloca(AV_ERROR_MAX_STRING_SIZE)), \
    AV_ERROR_MAX_STRING_SIZE, errnum)
#endif

using namespace sgct;

const std::string& Capture::getVideoHost() const {
    return mVideoHost;
}

int Capture::getWidth() const {
    return mWidth;
}

int Capture::getHeight() const {
    return mHeight;
}

const std::string& Capture::getFormat() const {
    return mVideoStrFormat;
}

size_t Capture::getNumberOfDecodedFrames() const {
    return mDecodedVideoFrames;
}

bool Capture::init() {
    if (mVideoDevice.empty()) {
        MessageHandler::printError("No video device specified");
        cleanup();
        return false;
    }

    initFFmpeg();
    setupOptions();

    //check out https://ffmpeg.org/ffmpeg-devices.html
    AVInputFormat* iformat;
    std::string inputName;

#ifdef __WIN32__
    iformat = av_find_input_format("dshow");
    inputName = "video=" + mVideoDevice;
#elif defined __APPLE__
    iformat = av_find_input_format("avfoundation");
    inputName = mVideoDevice;
#else  //linux NOT-Tested
    iformat = av_find_input_format("video4linux2");
    inputName = mVideoDevice;
#endif

    const int openSuccess = avformat_open_input(
        &mFMTContext,
        inputName.c_str(),
        iformat,
        &mOptions
    );
    if (openSuccess < 0) {
        MessageHandler::printError("Could not open capture input");
        cleanup();
        return false;
    }

    // retrieve stream information
    const int findSuccess = avformat_find_stream_info(mFMTContext, nullptr);
    if (findSuccess < 0) {
        MessageHandler::printError("Could not find stream information");
        cleanup();
        return false;
    }

    if (!initVideoStream()) {
        cleanup();
        return false;
    }

    // dump format info to console
    av_dump_format(mFMTContext, 0, inputName.c_str(), 0);

    if (mVideoCodecContext) {
        if (!allocateVideoDecoderData(mVideoCodecContext->pix_fmt)) {
            cleanup();
            return false;
        }
    }

    // initialize packet, set data to nullptr, let the demuxer fill it
    av_init_packet(&mPkt);
    mPkt.data = nullptr;
    mPkt.size = 0;

    // success
    mInited = true;

    MessageHandler::printInfo("Capture init complete");

    return true;
}

void Capture::setVideoHost(std::string hostAddress) {
    mVideoHost = std::move(hostAddress);
}

void Capture::setVideoDevice(std::string videoDeviceName) {
    mVideoDevice = std::move(videoDeviceName);
}

void Capture::setVideoDecoderCallback(
                            std::function<void(uint8_t** data, int width, int height)> cb)
{
    mVideoDecoderCallback = std::move(cb);
}

void Capture::addOption(std::pair<std::string, std::string> option) {
    mUserOptions.push_back(std::move(option));
}

bool Capture::poll() {
    if (!mInited) {
        return false;
    }
    

    bool all_ok = true;

    const int readSuccess = av_read_frame(mFMTContext, &mPkt);
    if (readSuccess >= 0) {
        AVPacket orig_pkt = mPkt;

        do {
            int gotVideoFrame;
            const int ret = decodePacket(&gotVideoFrame);
            if (ret < 0) {
                MessageHandler::printError("Failed to decode package");
                all_ok = false;
                break;
            }

            mPkt.data += ret;
            mPkt.size -= ret;
        } while (mPkt.size > 0);

        av_free_packet(&orig_pkt);
    }

    return all_ok;
}

void Capture::setupOptions() {
    // reduce latency
    av_dict_set(&mOptions, "fflags", "nobuffer", 0);
    // reduce the latency by flushing out packets immediately
    av_dict_set(&mOptions, "fflags", "flush_packets", 0);

    // set user options
    for (size_t i = 0; i < mUserOptions.size(); i++) {
        if (!mUserOptions.at(i).first.empty() && !mUserOptions.at(i).second.empty()) {
            av_dict_set(
                &mOptions,
                mUserOptions.at(i).first.c_str(),
                mUserOptions.at(i).second.c_str(),
                0
            );
        }
    }
}

void Capture::initFFmpeg() {
    // set log level
    av_log_set_level(AV_LOG_QUIET);

    // register all formats and mCodecs
    av_register_all();
    avdevice_register_all();
}

bool Capture::initVideoStream() {
    // open video stream
    const int res  = openCodeContext(mFMTContext, AVMEDIA_TYPE_VIDEO, mVideo_stream_idx);
    if (res >= 0) {
        mVideoStream = mFMTContext->streams[mVideo_stream_idx];
        mVideoCodecContext = mVideoStream->codec;

        AVCodecID codecId = mVideoCodecContext->codec_id;

        mWidth = mVideoCodecContext->width;
        mHeight = mVideoCodecContext->height;
        return true;
    }
    else {
        return false;
    }
}

int Capture::openCodeContext(AVFormatContext* fmt_ctx, enum AVMediaType type,
                             int& streamIndex)
{
    AVStream* st;
    AVCodecContext* dec_ctx = nullptr;
    AVCodec* dec = nullptr;
    AVDictionary* opts = nullptr;

    const int ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if (ret < 0) {
        MessageHandler::printError(
            "Could not find %s stream", av_get_media_type_string(type)
        );
        return ret;
    }
    else {
        streamIndex = ret;
        st = fmt_ctx->streams[streamIndex];

        // find decoder for the stream
        dec_ctx = st->codec;
        dec_ctx->thread_count = 0; // auto number of threads
        dec_ctx->flags |= CODEC_FLAG_LOW_DELAY;
        dec_ctx->flags2 |= CODEC_FLAG2_FAST;

        dec = avcodec_find_decoder(dec_ctx->codec_id);

        if (!dec) {
            MessageHandler::printError(
                "Could not find %s codec", av_get_media_type_string(type)
            );
            return AVERROR(EINVAL);
        }

        // Init the decoders, with or without reference counting
#if USE_REF_COUNTER
        av_dict_set(&opts, "refcounted_frames", "1", 0);
#else
        av_dict_set(&opts, "refcounted_frames", "0", 0);
#endif
        const int retOpen = avcodec_open2(dec_ctx, dec, &opts);
        if (retOpen < 0) {
            MessageHandler::printError(
                "Could not open %s codec", av_get_media_type_string(type)
            );
            return ret;
        }
    }

    return 0;
}

bool Capture::allocateVideoDecoderData(AVPixelFormat pix_fmt) {
    if (pix_fmt != mDstPixFmt) {
        constexpr const int BufferSize = 256;
        char Buffer[BufferSize];

        mVideoStrFormat = av_get_pix_fmt_string(Buffer, 256, pix_fmt);
        const size_t formatFound = mVideoStrFormat.find(' ');
        if (formatFound != std::string::npos) {
            mVideoStrFormat = mVideoStrFormat.substr(0, formatFound);
        }

        mVideoDstFormat = av_get_pix_fmt_string(Buffer, 256, mDstPixFmt);
        const size_t dstFound = mVideoDstFormat.find(' ');
        if (dstFound != std::string::npos) {
            mVideoDstFormat = mVideoDstFormat.substr(0, dstFound);
        }

        MessageHandler::printInfo(
            "Creating video scaling context (%s->%s)",
            mVideoStrFormat.c_str(), mVideoDstFormat.c_str()
        );

        // create context for frame convertion
        mVideoScaleContext = sws_getContext(
            mWidth,
            mHeight,
            pix_fmt,
            mWidth,
            mHeight,
            mDstPixFmt,
            SWS_FAST_BILINEAR,
            nullptr,
            nullptr,
            nullptr
        );
        if (!mVideoScaleContext) {
            MessageHandler::printError("Could not allocate frame convertion context");
            return false;
        }
    }

    mTempFrame = av_frame_alloc();
    if (!mTempFrame) {
        MessageHandler::printError("Could not allocate temp frame data");
        return false;
    }

    mTempFrame->width = mWidth;
    mTempFrame->height = mHeight;
    mTempFrame->format = mDstPixFmt;

    const int ret = av_image_alloc(
        mTempFrame->data,
        mTempFrame->linesize,
        mWidth,
        mHeight,
        mDstPixFmt,
        1
    );
    if (ret < 0) {
        MessageHandler::printError("Could not allocate temp frame buffer");
        return false;
    }

    if (!mFrame) {
        mFrame = av_frame_alloc();
    }

    if (!mFrame) {
        MessageHandler::printError("Could not allocate frame data");
        return false;
    }

    return true;
}

int Capture::decodePacket(int* gotVideoPtr) {
    int decoded = mPkt.size;
    bool packageOk = false;

    *gotVideoPtr = 0;
    
    if (mPkt.stream_index == mVideo_stream_idx && mVideoCodecContext) {
        // decode video frame
        const int decodeSuccess = avcodec_decode_video2(
            mVideoCodecContext,
            mFrame,
            gotVideoPtr,
            &mPkt
        );

        if (decodeSuccess < 0) {
            MessageHandler::printError(
                "Video decoding error: %s", av_err2str(decodeSuccess)
            );
            return decodeSuccess;
        }

        decoded = FFMIN(decodeSuccess, mPkt.size);
        if (*gotVideoPtr) {
            packageOk = true;
            
            if (mVideoCodecContext->pix_fmt != mDstPixFmt) {
                // convert to destination pixel format
                const int scaleSuccess = sws_scale(
                    mVideoScaleContext,
                    mFrame->data,
                    mFrame->linesize,
                    0,
                    mHeight,
                    mTempFrame->data,
                    mTempFrame->linesize
                );
                if (scaleSuccess < 0) {
                    MessageHandler::printError(
                        "Failed to convert decoded frame to %s", mVideoDstFormat.c_str()
                    );
                    return scaleSuccess;
                }
            }
            else {
                const int copySuccess = av_frame_copy(mTempFrame, mFrame);
                if (copySuccess < 0) {
                    MessageHandler::printError("Failed to copy frame");
                    return copySuccess;
                }
            }

            //store mTempFrame;
            if (mVideoDecoderCallback) {
                mVideoDecoderCallback(mTempFrame->data, mWidth, mHeight);
            }

            mDecodedVideoFrames++;
        }
    }

#if USE_REF_COUNTER
    // If we use the new API with reference counting, we own the data and need to
    // de-reference it when we don't use it anymore
    if (packageOk) {
        av_frame_unref(mFrame);
    }
#endif

    return decoded;
}

void Capture::cleanup() {
    mInited = false;
    mVideoDecoderCallback = nullptr;

    if (mVideoCodecContext) {
        avcodec_close(mVideoCodecContext);
        mVideoCodecContext = nullptr;
    }

    if (mFMTContext) {
        avformat_close_input(&mFMTContext);
        mFMTContext = nullptr;
    }

    if (mFrame) {
        av_frame_free(&mFrame);
        mFrame = nullptr;
    }

    if (mTempFrame) {
        av_frame_free(&mTempFrame);
        mTempFrame = nullptr;
    }

    if (mVideoScaleContext) {
        sws_freeContext(mVideoScaleContext);
        mVideoScaleContext = nullptr;
    }

    mVideoStream = nullptr;

    mWidth = 0;
    mHeight = 0;
}
