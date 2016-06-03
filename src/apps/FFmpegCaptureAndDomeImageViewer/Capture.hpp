#ifndef __FFMPEG_CAPTURE_
#define __FFMPEG_CAPTURE_

extern "C"
{
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <string>
#include <vector>
#include <functional>

class Capture
{
public:
	Capture();
	~Capture();
	bool init();
    void setVideoHost(std::string hostAdress);
	void setVideoDevice(std::string videoDeviceName);
	void setVideoDecoderCallback(std::function<void(uint8_t ** data, int width, int height)> cb);
	void addOption(std::pair<std::string, std::string> option);
	bool poll();

    std::string getVideoHost() const;
	int getWidth() const;
	int getHeight() const;
	const char * getFormat() const;
	std::size_t getNumberOfDecodedFrames() const;

private:
	void initFFmpeg();
	bool initVideoStream();
	int openCodeContext(AVFormatContext *fmt_ctx, enum AVMediaType type, int & streamIndex);
	bool allocateVideoDecoderData(AVPixelFormat pix_fmt);
	int decodePacket(int * gotVideoPtr);
	void setupOptions();
	void cleanup();

	AVDictionary		* mOptions;
	AVFormatContext		* mFMTContext;
	AVStream			* mVideoStream;
	AVCodecContext		* mVideoCodecContext;
	SwsContext			* mVideoScaleContext;
	AVFrame				* mFrame; //holds src format frame
	AVFrame				* mTempFrame; //holds dst format frame

	AVPixelFormat mDstPixFmt;
	AVPacket mPkt;

	std::size_t mDecodedVideoFrames;
	bool mInited;

	int mWidth;
	int mHeight;
	int mVideo_stream_idx;

    std::string mVideoHost;
	std::string mVideoDevice;
	std::string mVideoDstFormat;
	std::string mVideoStrFormat;
	std::vector< std::pair<std::string, std::string> > mUserOptions;

	//callback function pointer
	std::function<void(uint8_t ** data, int width, int height)> mVideoDecoderCallback;
};

#endif