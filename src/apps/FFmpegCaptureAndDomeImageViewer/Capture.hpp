#ifndef __CAPTURE_H__
#define __CAPTURE_H__

extern "C" {
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
} // extern "C"

#include <functional>
#include <string>
#include <vector>

class Capture {
public:
    bool init();
    void cleanup();
    void setVideoHost(std::string hostAdress);
    void setVideoDevice(std::string videoDeviceName);
    void setVideoDecoderCallback(
        std::function<void(uint8_t** data, int width, int height)> cb
    );
    void addOption(std::pair<std::string, std::string> option);
    bool poll();

    const std::string& getVideoHost() const;
    int getWidth() const;
    int getHeight() const;
    const std::string& getFormat() const;
    size_t getNumberOfDecodedFrames() const;

private:
    void initFFmpeg();
    bool initVideoStream();
    int openCodeContext(AVFormatContext* fmt_ctx, enum AVMediaType type,
        int& streamIndex);
    bool allocateVideoDecoderData(AVPixelFormat pix_fmt);
    int decodePacket(int * gotVideoPtr);
    void setupOptions();

    AVDictionary* mOptions = nullptr;
    AVFormatContext* mFMTContext = nullptr;
    AVStream* mVideoStream = nullptr;
    AVCodecContext* mVideoCodecContext = nullptr;
    SwsContext* mVideoScaleContext = nullptr;
    AVFrame* mFrame = nullptr; // holds src format frame
    AVFrame* mTempFrame = nullptr; // holds dst format frame

    AVPixelFormat mDstPixFmt = AV_PIX_FMT_BGR24;
    AVPacket mPkt;

    size_t mDecodedVideoFrames = 0;
    bool mInited = false;

    int mWidth = 0;
    int mHeight = 0;
    int mVideo_stream_idx = -1;

    std::string mVideoHost;
    std::string mVideoDevice;
    std::string mVideoDstFormat;
    std::string mVideoStrFormat;
    std::vector<std::pair<std::string, std::string>> mUserOptions;

    // callback function pointer
    std::function<void(uint8_t** data, int width, int height)> mVideoDecoderCallback;
};

#endif // __CAPTURE_H__
