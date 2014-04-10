/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_NETWORK
#define _SGCT_NETWORK
#include <string>
#include <vector>
#include "helpers/SGCTCPPEleven.h"

#define MAX_NET_SYNC_FRAME_NUMBER 10000

#ifdef __WIN32__
    typedef size_t SGCT_SOCKET;
    #if (_MSC_VER >= 1100) //visual studio 5 or later (basically all visual studio versions)
        typedef int ssize_t;
    #endif
#else //linux & OS X
    typedef int SGCT_SOCKET;
#endif

#include "external/tinythread.h"

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
	enum ConnectionTypes { SyncConnection = 0, ExternalASCIIConnection, ExternalRawConnection };
	enum ReceivedIndex { Current = 0, Previous };

	SGCTNetwork();
	void init(const std::string port, const std::string address, bool _isServer, int id, ConnectionTypes serverType, bool firmSync);
	void closeNetwork(bool forced);
	void initShutdown();
#ifdef __LOAD_CPP11_FUN__
	void setDecodeFunction(sgct_cppxeleven::function<void (const char*, int, int)> callback);
	void setUpdateFunction(sgct_cppxeleven::function<void (int)> callback);
	void setConnectedFunction(sgct_cppxeleven::function<void (void)> callback);
#endif
	void setBufferSize(unsigned int newSize);
	void setConnectedStatus(bool state);
	void setOptions(SGCT_SOCKET * socketPtr);
	void closeSocket(SGCT_SOCKET lSocket);

	ConnectionTypes getTypeOfConnection();
	const int getId();
	bool isServer();
	bool isConnected();
	bool isTerminated();
	int getSendFrame(ReceivedIndex ri = Current);
	int getRecvFrame(ReceivedIndex ri);
	double getLoopTime();
	bool isUpdated();
	void setRecvFrame(int i);
	void sendData(void * data, int length);
	void sendStr(std::string msg);
	static ssize_t receiveData(SGCT_SOCKET & lsocket, char * buffer, int length, int flags);
	static int parseInt(char * str);
	static unsigned int parseUnsignedInt(char * str);
	int iterateFrameCounter();
	void pushClientMessage();
	std::string getPort();
	std::string getAddress();

	SGCT_SOCKET mSocket;
	SGCT_SOCKET mListenSocket;
#ifdef __LOAD_CPP11_FUN__
	sgct_cppxeleven::function< void(const char*, int, int) > mDecoderCallbackFn;
	sgct_cppxeleven::function< void(int) > mUpdateCallbackFn;
	sgct_cppxeleven::function< void(void) > mConnectedCallbackFn;
#endif

    bool mTerminate; //set to true upon exit

	int mBufferSize;
	int mRequestedSize;
	static const std::size_t mHeaderSize = 9;

	tthread::mutex mConnectionMutex;
	tthread::condition_variable mStartConnectionCond;
	tthread::thread * mCommThread;
	tthread::thread * mMainThread;

private:
	enum timeStampIndex { Send = 0, Total };

	bool mFirmSync, mUpdated;
	ConnectionTypes mConnectionType;
	bool mServer;
	bool mConnected;
	int mSendFrame[2];
	int mRecvFrame[2];
	double mTimeStamp[2];
	int mId;
	std::string mPort;
	std::string mAddress;
};
}

#endif
