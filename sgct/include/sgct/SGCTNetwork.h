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
#include <functional>

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
	void init(const std::string port, const std::string ip, bool _isServer, unsigned int numberOfNodesInConfig);
	void sync();
	void close();
	bool matchHostName(const std::string name);
	bool matchAddress(const std::string ip);
	void setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback);
	
	inline bool isRunning() { return mRunning; }
	inline bool isServer() { return mServer; }
	inline bool areAllNodesConnected() { return mAllNodesConnected; }
	inline unsigned int getNumberOfNodesInConfig() { return mNumberOfNodesInConfig; }
	void setRunning(bool state) { mRunning = state; }
	void setAllNodesConnected(bool state);
	
	enum PackageHeaders { SyncHeader = 0, SizeHeader, ClusterConnected };
	SOCKET mSocket;
	std::tr1::function< void(const char*, int, int) > mDecoderCallbackFn;
	std::vector<ConnectionData> clients;
	unsigned int mBufferSize;

private:
	void sendStrToAllClients(const std::string str);
	void sendDataToAllClients(void * data, int lenght);

	bool mRunning;
	bool mServer;
	bool mAllNodesConnected;
	unsigned int mNumberOfNodesInConfig;
	std::string hostName;
	std::vector<std::string> localAddresses;
	int threadID;
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