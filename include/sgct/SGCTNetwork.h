/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_NETWORK
#define _SGCT_NETWORK
#include <string>
#include <vector>

#if (_MSC_VER >= 1400 || _XCODE) //visual studio 2005 or later
#include <functional>
#else
#include <tr1/functional>
#endif

#if (_MSC_VER >= 1700 || _XCODE) //visual studio 2012 or later
namespace sgct_cppxeleven = std;
#else
namespace sgct_cppxeleven = std::tr1;
#endif

#define MAX_NET_SYNC_FRAME_NUMBER 10000

#ifdef __WIN32__
	typedef unsigned int SGCT_SOCKET;
	typedef int ssize_t;
#else
	typedef int SGCT_SOCKET;
#endif

typedef void * GLFWmutex;
typedef void * GLFWcond;

namespace sgct_core //small graphics cluster toolkit
{

/*!
SGCTNetwork manages peer-to-peer tcp connections.
*/
class SGCTNetwork
{
public:
	//ASCII device control chars = 17, 18, 19 & 20
	enum PackageHeaders { SyncByte = 17, ConnectedByte, DisconnectByte, FillByte };
	enum ServerTypes { SyncServer = 0, ExternalControl };
	enum ReceivedIndex { Current = 0, Previous };

	SGCTNetwork();
	void init(const std::string port, const std::string ip, bool _isServer, int id, int serverType);
	void closeNetwork(bool forced);
	void initShutdown();
	void setDecodeFunction(sgct_cppxeleven::function<void (const char*, int, int)> callback);
	void setUpdateFunction(sgct_cppxeleven::function<void (int)> callback);
	void setConnectedFunction(sgct_cppxeleven::function<void (void)> callback);
	void setBufferSize(unsigned int newSize);
	void setConnectedStatus(bool state);
	void setOptions(SGCT_SOCKET * socketPtr);
	void closeSocket(SGCT_SOCKET lSocket);

	int getTypeOfServer();
	int getId();
	bool isServer();
	bool isConnected();
	bool isTerminated();
	int getSendFrame();
	int getRecvFrame(ReceivedIndex ri);
	bool isUpdated();
	void setRecvFrame(int i);
	void sendData(void * data, int length);
	void sendStr(std::string msg);
	static ssize_t receiveData(SGCT_SOCKET & lsocket, char * buffer, int length, int flags);
	static int parseInt(char * str);
	static unsigned int parseUnsignedInt(char * str);
	int iterateFrameCounter();
	void pushClientMessage();

	SGCT_SOCKET mSocket;
	SGCT_SOCKET mListenSocket;
	sgct_cppxeleven::function< void(const char*, int, int) > mDecoderCallbackFn;
	sgct_cppxeleven::function< void(int) > mUpdateCallbackFn;
	sgct_cppxeleven::function< void(void) > mConnectedCallbackFn;
	int mCommThreadId;

    bool mTerminate; //set to true upon exit

	std::size_t mBufferSize;
	std::size_t mRequestedSize;
	static const std::size_t mHeaderSize = 9;

	GLFWmutex mConnectionMutex;
	GLFWcond mDoneCond;
	GLFWcond mStartConnectionCond;

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
