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

#ifndef _SGCT_NETWORK
#define _SGCT_NETWORK
#include <string>
#include <vector>

#if (_MSC_VER >= 1400) //visual studio 2005 or later
#include <functional>
#else
#include <tr1/functional>
#endif

#define MAX_NET_SYNC_FRAME_NUMBER 10000

#ifdef __WIN32__
	typedef unsigned int SOCKET;
#else
	typedef int SOCKET;
#endif

namespace core_sgct //small graphics cluster toolkit
{

class SGCTNetwork
{
public:
	SGCTNetwork();
	void init(const std::string port, const std::string ip, bool _isServer, int id, int serverType);
	void closeNetwork();
	void setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback);
	void setUpdateFunction(std::tr1::function<void (int,bool)> callback);
	void setConnectedFunction(std::tr1::function<void (void)> callback);
	void setBufferSize(unsigned int newSize);
	void setConnectedStatus(bool state);
	void setOptions(SOCKET * socketPtr);
	void closeSocket(SOCKET lSocket);

	int getTypeOfServer();
	int getId();
	bool isServer();
	bool isConnected();
	int getSendFrame();
	bool compareFrames();
	void setRecvFrame(int i);
	void swapFrames();
	int sendData(void * data, int length);
	int sendStr(std::string msg);
	void iterateFrameCounter();
	void checkIfBufferNeedsResizing();
	void pushClientMessage();

	SOCKET mSocket;
	SOCKET mListenSocket;
	std::tr1::function< void(const char*, int, int) > mDecoderCallbackFn;
	std::tr1::function< void(int,bool) > mUpdateCallbackFn;
	std::tr1::function< void(void) > mConnectedCallbackFn;
	int mCommThreadId;

	unsigned int mBufferSize;
	unsigned int mRequestedSize;
	static const unsigned int syncHeaderSize = 9;
	//ASCII device control chars = 17, 18, 19 & 20
	enum PackageHeaders { SyncHeader = 17, SizeHeader, ConnectedHeader };
	enum ServerTypes { SyncServer = 0, ExternalControl };

private:
	int mServerType;
	bool mServer;
	int mMainThreadId;
	bool mConnected;
	int mSendFrame;
	int mRecvFrame[2];
	int mId;
};
}

#endif
