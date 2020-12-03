/*************************************************************************
Copyright (c) 2017 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _NDI_H
#define _NDI_H

#include <Processing.NDI.Lib.h>
#include <string>

#define NUMBER_OF_NDI_BUFFERS 2

class NDISender {
public:
	enum PixelFMT { BGRA, UYVY };

	NDISender() = default;
	~NDISender();
	bool init(int width, int height, const std::string & name, PixelFMT format = UYVY);
	void submitFrame(unsigned char * frame, int channels);
	inline bool isBGRA() { return mPixFMT == BGRA; }

private:
	static void workerBGRA(unsigned char * src, unsigned char * dst, int start, int strideSize, int height, int count);
	static void workerUYVY(unsigned char * src, unsigned char * dst, int start, int strideSize, int height, int count);

	int mWidth = 0;
	int mHeight = 0;
	int mStrideSize = 0;
	int mFrameSize = 0;
	int mIndex = 0;
	bool mValid = false;
	PixelFMT mPixFMT = PixelFMT::BGRA;
	NDIlib_send_instance_t mNDI_send = nullptr;
	NDIlib_video_frame_t mNDI_video_frame[NUMBER_OF_NDI_BUFFERS];
};

#endif