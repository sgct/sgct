/*************************************************************************
Copyright (c) 2017 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "sgct.h"
#include "ndi.h"
#include <future>

NDISender::NDISender()
{
	mWidth = 0;
	mHeight = 0;
	mStrideSize = 0;
	mFrameSize = 0;
	mIndex = 0;
	mValid = false;
	mNDI_send = NULL;
}

NDISender::~NDISender()
{
	//sync
	NDIlib_send_send_video_async(mNDI_send, NULL); // Sync here

	for (unsigned int i = 0; i < NUMBER_OF_NDI_BUFFERS; ++i)
	{
		delete[] mNDI_video_frame[i].p_data;
	}

	// Destroy the NDI sender
	NDIlib_send_destroy(mNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "NDI destroyed successfully!\n");
}

bool NDISender::init(int width, int height, const std::string & name, PixelFMT format)
{
	mWidth = width;
	mHeight = height;
	mPixFMT = format;
	
	// Not required, but "correct" (see the SDK documentation.
	if (!NDIlib_initialize())
	{	// Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
		// you can check this directly with a call to NDIlib_is_supported_CPU()
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Cannot run NDI.\n");
		return false;
	}

	// Create an NDI source that is not locked to any clock
	const NDIlib_send_create_t NDI_send_create_desc = { name.c_str(), NULL, true, false };

	// We create the NDI sender
	mNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!mNDI_send)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Cannot create NDI sender.\n");
		return false;
	}

	// Provide a meta-data registration that allows people to know what we are. Note that this is optional.
	// Note that it is possible for senders to also register their preferred video formats.
	static const char* p_connection_string = "<ndi_product long_name=\"SGCT test sender\" "
		"             short_name=\"SGCT\" "
		"             manufacturer=\"SGCT\" "
		"             version=\"1.000.000\" "
		"             session=\"default\" "
		"             model_name=\"S1\" "
		"             serial=\"ABCDEFG\"/>";

	const NDIlib_metadata_frame_t NDI_connection_type = {
		// The length
		(int)::strlen(p_connection_string),
		// Timecode (synthesized for us !)
		NDIlib_send_timecode_synthesize,
		// The string
		(CHAR*)p_connection_string
	};
	NDIlib_send_add_connection_metadata(mNDI_send, &NDI_connection_type);

	mStrideSize = mPixFMT == BGRA ? width * 4 : width * 2;
	mFrameSize = mStrideSize * height;
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	for (int i = 0; i < NUMBER_OF_NDI_BUFFERS; ++i)
	{
		mNDI_video_frame[i] = {
			// Resolution
			width, height,
			//color space
			mPixFMT == BGRA ? NDIlib_FourCC_type_BGRA : NDIlib_FourCC_type_UYVY,
			// The frame-rate (assume 30 fps)
			30000, 1000,
			// The aspect ratio (16:9)
			aspectRatio,
			// This is a progressive frame
			NDIlib_frame_format_type_progressive,
			// Timecode (synthesized for us !)
			NDIlib_send_timecode_synthesize,
			// The video memory used for this frame
			new BYTE[mFrameSize],
			// The line to line stride of this image
			mStrideSize
		};
	}

	//all ok
	mValid = true;
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "NDI initiated successfully!\n");
	return true;
}

void NDISender::submitFrame(unsigned char * frame, int channels)
{
	if (mValid)
	{		
		//sanity check
		if (channels == 4 && mPixFMT != BGRA)
			return;

		if (channels == 3 && mPixFMT != UYVY)
			return;

		
		unsigned char * dst = reinterpret_cast<unsigned char *>(mNDI_video_frame[mIndex].p_data);

		if (mHeight > 720 || mPixFMT == UYVY)
		{
			unsigned int chunks = std::thread::hardware_concurrency();
			std::vector<std::future<void>> futures;

			int count = mHeight / chunks;
			for (unsigned int i = 0; i < chunks; ++i)
			{
				int start = i * count;
				futures.push_back(std::async(
					mPixFMT == BGRA ? workerBGRA : workerUYVY,
					frame, dst,
					start, mStrideSize,
					mHeight, count));
			}

			//sync
			for (auto &e : futures)
			{
				e.wait();
			}
		}
		else //no multithreading
		{
			if (mPixFMT == BGRA)
				workerBGRA(frame, dst, 0, mStrideSize, mHeight, mHeight);
			else
				workerUYVY(frame, dst, 0, mStrideSize, mHeight, mHeight);
		}
		
		//send
		NDIlib_send_send_video_async(mNDI_send, &mNDI_video_frame[mIndex]);

		//iterate
		mIndex = (mIndex + 1) % NUMBER_OF_NDI_BUFFERS;
	}
}

void NDISender::workerBGRA(unsigned char * src, unsigned char * dst, int start, int strideSize, int height, int count)
{
	//copy data and flip vertically
	int offset_src, offset_dst;

	for (int row = start; row < (start + count); ++row)
	{
		offset_src = (height - row - 1) * strideSize;
		offset_dst = row * strideSize;
		memcpy(dst + offset_dst, src + offset_src, strideSize);
	}
}

void NDISender::workerUYVY(unsigned char * src, unsigned char * dst, int start, int strideSize, int height, int count)
{
	//flip & convert
	unsigned int dst_index = 0;
	unsigned int src_index = 0;
	unsigned int inverted_row = 0;

	//constants
	//----------------------
	//ITU-R BT.601 (SD)
	//const float Kr = 0.299f; //red
	//const float Kb = 0.114f; //blue

	//ITU-R BT.709 (HD)
	const float Kr = 0.2126f; //red
	const float Kb = 0.0722f; //blue

	//ITU-R BT.2020 (UHD)
	//const float Kr = 0.2627f; //red
	//const float Kb = 0.0593f; //blue

	const float Kg = 1.0f - Kr - Kb; //green
	const float KbMult = (1.0f - Kb)*2.0f;
	const float KrMult = (1.0f - Kr)*2.0f;

	float y1, y2, u, v, r1, g1, b1, r2, g2, b2;
	int w = strideSize / 2;

	for (int row = start; row < (start + count); ++row)
	{
		inverted_row = height - (row + 1);
		for (int col = 0; col < w; col += 2)
		{
			src_index = (w * inverted_row + col) * 3;
			dst_index = (w * row + col) * 2;

			b1 = static_cast<float>(src[src_index]);
			g1 = static_cast<float>(src[src_index + 1]);
			r1 = static_cast<float>(src[src_index + 2]);

			b2 = static_cast<float>(src[src_index + 3]);
			g2 = static_cast<float>(src[src_index + 4]);
			r2 = static_cast<float>(src[src_index + 5]);

			y1 = r1 * Kr + g1 * Kg + b1 * Kb;
			y2 = r2 * Kr + g2 * Kg + b2 * Kb;
			u = (b1 - y1) / KbMult + 128.0f;
			v = (r1 - y1) / KrMult + 128.0f;

			dst[dst_index] = static_cast<BYTE>(u);
			dst[dst_index + 1] = static_cast<BYTE>(y1);
			dst[dst_index + 2] = static_cast<BYTE>(v);
			dst[dst_index + 3] = static_cast<BYTE>(y2);
		}
	}
}