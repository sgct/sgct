/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

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
	void init();
	void setWindowTitle(const char * title);
	void setWindowResolution(const int x, const int y);
	void initWindowResolution(const int x, const int y);
	bool isWindowResized();
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
	int mWindowResOld[2];
	int mWindowMode;
};
}

#endif
