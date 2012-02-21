/*
	Window.h

	© 2012 Miroslav Andel
*/

#ifndef _SGCT_WINDOW_H_
#define _SGCT_WINDOW_H_

namespace core_sgct
{
/*!
Helper class for window data
*/
class SGCTWindow
{
public:
	SGCTWindow();
	void close();
	void init(bool lockVerticalSync);
	void setWindowTitle(const char * title);
	void setWindowResolution(const int x, const int y);
	void setWindowPosition(const int x, const int y);
	void setWindowMode(const int mode);
	void setBarrier(const bool state);
	void useSwapGroups(const bool state);
	void useQuadbuffer(const bool state);
	bool openWindow();
	void initNvidiaSwapGroups();
	void getSwapGroupFrameNumber(unsigned int & frameNumber);
	void resetSwapGroupFrameNumber();
	
	inline bool isBarrierActive() { return mBarrier; }
	inline bool isUsingSwapGroups() { return mUseSwapGroups; }
	inline bool isSwapGroupMaster() { return mSwapGroupMaster; }
	inline int getWindowMode() { return mWindowMode; }
	inline int getHResolution() { return mWindowRes[0]; }
	inline int getVResolution() { return mWindowRes[1]; }

private:
	bool mUseSwapGroups;
	bool mBarrier;
	bool mSwapGroupMaster;
	bool mUseQuadBuffer;
	int mWindowRes[2];
	int mWindowPos[2];
	int mWindowMode;
};
}

#endif