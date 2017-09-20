/*******************************************************************************

Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h

*******************************************************************************/

#ifndef __RGBEASY_CAPTURE_
#define __RGBEASY_CAPTURE_

#include <string>

class RGBEasyCapture
{
public:
	RGBEasyCapture();
	~RGBEasyCapture();
	bool initialize();
	void deinitialize();

	bool initializeGL();
	void deinitializeGL();

	void runCapture();

	bool prepareForRendering();
	void renderingCompleted();

    void setCaptureHost(std::string hostAdress);
	void setCaptureInput(int input);
	void setCaptureGanging(bool doGanging);

	unsigned long getWidth();
	unsigned long getHeight();
	bool getGanging();

    std::string getCaptureHost() const;

private:
    std::string mCaptureHost;
	bool mCaptureGanging;
};

#endif