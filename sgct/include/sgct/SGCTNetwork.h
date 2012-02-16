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

#if (_MSC_VER >= 1400) //visual studio 2005 or later
#include <functional>
#else
#include <tr1/functional>
#endif

namespace core_sgct //small graphics cluster toolkit
{

class ConnectionData
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
};

class SGCTNetwork
{
public:
	SGCTNetwork();
	void init(const std::string port, const std::string ip, bool _isServer, unsigned int numberOfNodesInConfig, int serverType = SyncServer);
	void sync();
	void close();
	bool matchHostName(const std::string name);
	bool matchAddress(const std::string ip);
	void setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback);
	void setBufferSize(unsigned int newSize);

	inline int getTypeOfServer() { return mServerType; }
	inline bool isRunning() { return mRunning; }
	inline bool isServer() { return mServer; }
	inline bool isClientConnected( int index ) { return (clients[index] != NULL && clients[index]->connected) ? true : false; }
	inline bool areAllNodesConnected() { return mAllNodesConnected; }
	inline unsigned int getNumberOfNodesInConfig() { return mNumberOfNodesInConfig; }
	void setRunningStatus(bool status) { mRunning = status; }
	void setClientConnectionStatus(int clientIndex, bool state);
	void setAllNodesConnected(bool state);
	void terminateClient(int index);
	void sendStrToAllClients(const std::string str);
	void sendDataToAllClients(void * data, int lenght);

	//ASCII device control chars = 17, 18, 19 & 20
	enum PackageHeaders { SyncHeader = 17, SizeHeader, ClusterConnected };
	enum ServerTypess { SyncServer = 0, ExternalControl };
	SOCKET mSocket;
	std::tr1::function< void(const char*, int, int) > mDecoderCallbackFn;
	std::vector<ConnectionData*> clients;
	
	unsigned int mBufferSize;
	unsigned int mRequestedSize;

private:

	int mServerType;
	bool mRunning;
	bool mServer;
	bool mAllNodesConnected;
	unsigned int mNumberOfNodesInConfig;
	std::string hostName;
	std::vector<std::string> localAddresses;
	int mainThreadID, pollClientStatusThreadID;
};

class TCPData
{
public:
	TCPData() { mClientIndex = -1; }

	SGCTNetwork * mNetwork;
	int mClientIndex; //-1 if message from server
};
}

#endif
