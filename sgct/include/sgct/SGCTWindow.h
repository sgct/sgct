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
	~SGCTWindow();
	void init(const char * windowTitle);
	void setWindowResolution(int x, int y);
	void setWindowPosition(int x, int y);
	void setWindowMode(int mode);
	void setBarrier(bool state);
	void useSwapGroups(bool state);
	void useQuadbuffer(bool state);
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