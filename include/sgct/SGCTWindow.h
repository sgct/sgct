/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_WINDOW_H_
#define _SGCT_WINDOW_H_

namespace sgct_core
{

/*!
Helper class for window data. 
*/
class SGCTWindow
{
public:
	SGCTWindow();
	void close();
	void init();
	void setWindowTitle(const char * title);
	void setWindowResolution(const int x, const int y);
	void setFramebufferResolution(const int x, const int y);
	void initWindowResolution(const int x, const int y);
	void swap();
	bool isWindowResized();
	void setWindowPosition(const int x, const int y);
	void setWindowMode(const int mode);
	void setBarrier(const bool state);
	void setFixResolution(const bool state);
	void useSwapGroups(const bool state);
	void useQuadbuffer(const bool state);
	bool openWindow();
	void initNvidiaSwapGroups();
	void getSwapGroupFrameNumber(unsigned int & frameNumber);
	void resetSwapGroupFrameNumber();

	/*!
		\returns If the frame buffer has a fix resolution this function returns true.
	*/
	inline bool isFixResolution() { return mUseFixResolution; }
	inline bool isBarrierActive() { return mBarrier; }
	inline bool isUsingSwapGroups() { return mUseSwapGroups; }
	inline bool isSwapGroupMaster() { return mSwapGroupMaster; }
	inline int getWindowMode() { return mWindowMode; }
	/*!
		\returns Get the horizontal window resolution.
	*/
	inline int getXResolution() { return mWindowRes[0]; }
	/*!
		\returns Get the vertical window resolution.
	*/
	inline int getYResolution() { return mWindowRes[1]; }
	/*!
		\returns Get the horizontal frame buffer resolution.
	*/
	inline int getXFramebufferResolution() { return mFramebufferResolution[0]; }
	/*!
		\returns Get the vertical frame buffer resolution.
	*/
	inline int getYFramebufferResolution() { return mFramebufferResolution[1]; }
	//! \returns the aspect ratio of the window 
	inline float getAspectRatio() { return mAspectRatio; }

private:
	bool mUseFixResolution;
	bool mUseSwapGroups;
	bool mBarrier;
	bool mSwapGroupMaster;
	bool mUseQuadBuffer;
	int mFramebufferResolution[2];
	int mWindowRes[2];
	int mWindowPos[2];
	int mWindowResOld[2];
	int mWindowMode;
	float mAspectRatio;
};
}

#endif
