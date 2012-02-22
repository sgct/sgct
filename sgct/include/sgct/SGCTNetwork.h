/* SGCTNetwork.h

© 2012 Miroslav Andel

*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _SGCT_NETWORK
#define _SGCT_NETWORK

#include <windows.h>
#include <winsock2.h>
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

/*class ConnectionData
{
public:
	ConnectionData()
	{
		connected = false;
		client_socket = INVALID_SOCKET;
		threadID = -1;
	}

	bool connected;
	SOCKET client_socket;
	int threadID;
};*/

class SGCTNetwork
{
public:
	SGCTNetwork();
	void init(const std::string port, const std::string ip, bool _isServer, int id, int serverType);
	//void sync();
	void close();
	//bool matchHostName(const std::string name);
	//bool matchAddress(const std::string ip);
	void setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback);
	void setDisconnectedFunction(std::tr1::function<void (void)> callback);
	void setBufferSize(unsigned int newSize);
	void setConnectedStatus(bool state);
	bool setNoDelay(SOCKET * socketPtr);

	inline int getTypeOfServer() { return mServerType; }
	inline int getId() { return mId; }
	//inline bool isRunning() { return mRunning; }
	inline bool isServer() { return mServer; }
	inline bool isConnected() { return mConnected; }
	//inline bool isClientConnected( int index ) { return (clients[index] != NULL && clients[index]->connected) ? true : false; }
	//inline bool areAllNodesConnected() { return mAllNodesConnected; }
	int getSendFrame();
	bool compareFrames();
	void setRecvFrame(int i) { mRecvFrame[0] = i; }
	void swapFrames();
	//unsigned int getFeedbackCount();
	//unsigned int getConnectedNodeCount();
	//void setRunningStatus(bool status) { mRunning = status; }
	//void setClientConnectionStatus(int clientIndex, bool state);
	//void setAllNodesConnected(bool state);
	//void terminateClient(int index);
	//void sendStrToAllClients(const std::string str);
	//void sendDataToAllClients(void * data, int lenght);
	void sendData(void * data, int lenght);
	void sendStr(std::string msg);
	void iterateFrameCounter();
	void checkIfBufferNeedsResizing();
	void pushClientMessage();
	static void setMutexState(bool lock);
	//void iterateFeedbackCounter();
	//void resetFeedbackCounter();

	SOCKET mSocket;
	SOCKET mListenSocket;
	std::tr1::function< void(const char*, int, int) > mDecoderCallbackFn;
	std::tr1::function< void(void) > mDisconnectedCallbackFn;
	
	unsigned int mBufferSize;
	unsigned int mRequestedSize;
	static const int syncHeaderSize = 5;
	//ASCII device control chars = 17, 18, 19 & 20
	enum PackageHeaders { SyncHeader = 17, SizeHeader };
	enum ServerTypes { SyncServer = 0, ExternalControl };

private:
	//void initAPI();

	int mServerType;
	//unsigned int mFeedbackCounter; //counts so that feedback is received from all nodes
	//unsigned int mNumberOfConnectedNodes;
	//bool mRunning;
	bool mServer;
	//bool mAllNodesConnected;
	//std::string hostName;
	//std::vector<std::string> localAddresses;
	int mainThreadID;//, pollClientStatusThreadID;
	bool mConnected;
	int mSendFrame;
	int mRecvFrame[2];
	int mId;
};

/*class TCPData
{
public:
	TCPData() { mClientIndex = -1; }

	SGCTNetwork * mNetwork;
	int mClientIndex; //-1 if message from server
};*/
}

#endif
