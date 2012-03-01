/* SGCTNetwork.h

� 2012 Miroslav Andel

*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _SGCT_NETWORK
#define _SGCT_NETWORK
#if WIN32
#include <windows.h>
#include <winsock2.h>
#else //Use BSD sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define SOCKET unsigned int
#endif
#include <string>
#include <vector>
#include <GL/glfw.h>

#if (_MSC_VER >= 1400) //visual studio 2005 or later
#include <functional>
#else
#include <tr1/functional>
#endif

#define MAX_NET_SYNC_FRAME_NUMBER 10000

namespace core_sgct //small graphics cluster toolkit
{

class SGCTNetwork
{
public:
	SGCTNetwork();
	void init(const std::string port, const std::string ip, bool _isServer, int id, int serverType);
	void closeSGCT();
	void setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback);
	void setUpdateFunction(std::tr1::function<void (int,bool)> callback);
	void setConnectedFunction(std::tr1::function<void (void)> callback);
	void setBufferSize(unsigned int newSize);
	void setConnectedStatus(bool state);
	bool setNoDelay(SOCKET * socketPtr);

	int getTypeOfServer();
	int getId();
	bool isServer();
	bool isConnected();
	int getSendFrame();
	bool compareFrames();
	void setRecvFrame(int i);
	void swapFrames();
	void sendData(void * data, int lenght);
	void sendStr(std::string msg);
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
	static const unsigned int syncHeaderSize = 5;
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
