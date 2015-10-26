/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_NETWORK
#define _SGCT_NETWORK
#include <string>
#include <vector>
#include <stdint.h>
#include "helpers/SGCTCPPEleven.h"

#define MAX_NET_SYNC_FRAME_NUMBER 10000

#if defined(__WIN32__) || defined(__MINGW32__) || defined(__MINGW64__)
    #define _WIN_PLATFORM
    typedef size_t SGCT_SOCKET;
#else //linux & OS X
    typedef int SGCT_SOCKET;
#endif

typedef int _ssize_t;

#ifndef SGCT_DONT_USE_EXTERNAL
	#include "external/tinythread.h"
#else
	#include <tinythread.h>
#endif

namespace sgct_core //small graphics cluster toolkit
{

/*!
SGCTNetwork manages peer-to-peer tcp connections.
*/
class SGCTNetwork
{
public:
	//ASCII device control chars = 17, 18, 19 & 20
	enum PackageHeaderId { DefaultId = 0, Ack = 6, DataId = 17, ConnectedId = 18, DisconnectId = 19, CompressedDataId = 21 };
	enum ConnectionTypes { SyncConnection = 0, ExternalASCIIConnection, ExternalRawConnection, DataTransfer };
	enum ReceivedIndex { Current = 0, Previous };

	SGCTNetwork();
	void init(const std::string port, const std::string address, bool _isServer, ConnectionTypes serverType);
	void closeNetwork(bool forced);
	void initShutdown();

#ifdef __LOAD_CPP11_FUN__
	void setDecodeFunction(sgct_cppxeleven::function<void (const char*, int, int)> callback);
	void setPackageDecodeFunction(sgct_cppxeleven::function<void(void*, int, int, int)> callback);
	void setUpdateFunction(sgct_cppxeleven::function<void (SGCTNetwork *)> callback);
	void setConnectedFunction(sgct_cppxeleven::function<void (void)> callback);
	void setAcknowledgeFunction(sgct_cppxeleven::function<void(int, int)> callback);
#endif
	void setBufferSize(uint32_t newSize);
	void setConnectedStatus(bool state);
	void setOptions(SGCT_SOCKET * socketPtr);
	void closeSocket(SGCT_SOCKET lSocket);

	ConnectionTypes getType();
	int getId() const;
	bool isServer();
	bool isConnected();

	bool isTerminated();
	int getSendFrame(ReceivedIndex ri = Current);
	int getRecvFrame(ReceivedIndex ri);
	double getLoopTime();
	bool isUpdated();
	void setRecvFrame(int i);
	void sendData(const void * data, int length);
	void sendStr(std::string msg);
    static int getLastError();
	static _ssize_t receiveData(SGCT_SOCKET & lsocket, char * buffer, int length, int flags);
	static int32_t parseInt32(char * str);
	static uint32_t parseUInt32(char * str);
	int iterateFrameCounter();
	void pushClientMessage();
	void enableNaglesAlgorithmInDataTransfer();
	std::string getPort();
	std::string getAddress();
	std::string getTypeStr();
	static std::string getTypeStr(ConnectionTypes ct);

#ifdef __LOAD_CPP11_FUN__
	sgct_cppxeleven::function< void(const char*, int, int) > mDecoderCallbackFn;
	sgct_cppxeleven::function< void(void*, int, int, int) > mPackageDecoderCallbackFn;
	sgct_cppxeleven::function< void(SGCTNetwork *) > mUpdateCallbackFn;
	sgct_cppxeleven::function< void(void) > mConnectedCallbackFn;
	sgct_cppxeleven::function< void(int, int) > mAcknowledgeCallbackFn;
#endif

private:
	void updateBuffer(char ** buffer, uint32_t requested_size, uint32_t & current_size);
	int readSyncMessage(char * _header, int32_t & _syncFrameNumber, uint32_t & _dataSize, uint32_t & _uncompressedDataSize);
	int readDataTransferMessage(char * _header, int32_t & _packageId, uint32_t & _dataSize, uint32_t & _uncompressedDataSize);
	int readExternalMessage();

	static void communicationHandlerStarter(void *arg);
	static void connectionHandlerStarter(void *arg);
	void communicationHandler();
	void connectionHandler();
	static bool parseDisconnectPackage(char * headerPtr);
	static std::string getUncompressionErrorAsStr(int err);

public:
	static const std::size_t mHeaderSize = 13;
	tthread::condition_variable mStartConnectionCond;

private:
	enum timeStampIndex { Send = 0, Total };

	SGCT_SOCKET mSocket;
	SGCT_SOCKET mListenSocket;

	ConnectionTypes mConnectionType;
	tthread::atomic<bool> mServer;
	tthread::atomic<bool> mConnected;
	tthread::atomic<bool> mUpdated;
	tthread::atomic<int32_t> mSendFrame[2];
	tthread::atomic<int32_t> mRecvFrame[2];
	tthread::atomic<bool> mTerminate; //set to true upon exit
	tthread::atomic<uint32_t> mRequestedSize;

	tthread::mutex mConnectionMutex;
	tthread::thread * mCommThread;
	tthread::thread * mMainThread;

	double mTimeStamp[2];
	int mId;
	uint32_t mBufferSize;
	uint32_t mUncompressedBufferSize;
	std::string mPort;
	std::string mAddress;

	char * mRecvBuf;
	char * mUncompressBuf;
	char mHeaderId;

	bool mUseNaglesAlgorithmInDataTransfer;
};
}

#endif
