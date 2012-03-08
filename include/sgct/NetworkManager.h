/* NetworkManager.h

© 2012 Miroslav Andel

*/

#ifndef _NETWORK_MANAGER_H_
#define _NETWORK_MANAGER_H_

#include "SGCTNetwork.h"
#include <vector>
#include <string>

namespace core_sgct
{

class NetworkManager
{
public:
	NetworkManager(int mode);
	~NetworkManager();
	bool init();
	void sync();
	bool isSyncComplete();
	void swapData();
	void close();

	bool matchHostName(const std::string name);
	bool matchAddress(const std::string ip);
	bool isComputerServer() { return mIsServer; }
	bool isRunning() { return mIsRunning; } 
	bool areAllNodesConnected() { return mAllNodesConnected; }
	SGCTNetwork * getExternalControlPtr() { return mIsExternalControlPresent ? mNetworkConnections[0] : NULL; }

	unsigned int getConnectedNodeCount() { return mNumberOfConnectedNodes; }

private:
	bool addConnection(const std::string port, const std::string ip, int serverType = SGCTNetwork::SyncServer);
	void initAPI();
	void getHostInfo();
	void updateConnectionStatus(int index, bool connected);
	void setAllNodesConnected();

public:
	enum ManagerMode { NotLocal = 0, LocalServer, LocalClient };
	static GLFWmutex gDecoderMutex;
	static GLFWcond gCond;
	static GLFWcond gStartConnectionCond;

private:
	std::vector<SGCTNetwork*> mNetworkConnections;
	
	std::string hostName; //stores this computers hostname
	std::vector<std::string> localAddresses; //stors this computers ip addresses
	
	bool mIsServer;
	bool mIsRunning;
	bool mIsExternalControlPresent;
	bool mAllNodesConnected;
	int mMode;
	unsigned int mNumberOfConnectedNodes;
};

}

#endif