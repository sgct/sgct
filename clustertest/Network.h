/* Network.h

© 2012 Miroslav Andel

*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _NETWORK
#define _NETWORK

#include <windows.h>
#include <winsock2.h>

#include <GL/glfw.h>
#include "SharedData.h"
#include <string>
#include <vector>

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

class Network
{
public:
	Network();
	void init(const std::string port, const std::string ip, bool _isServer, sgct::SharedData * _shdPtr);
	void sync();
	void close();
	bool matchHostName(const std::string name);
	bool matchAddress(const std::string ip);
	
	inline bool isRunning() { return running; }
	void setRunning(bool state) { running = state; }

	SOCKET mSocket;
	std::vector<ConnectionData> clients;
	sgct::SharedData * shdPtr;

private:
	void sendStrToAllClients(const std::string str);
	void sendDataToAllClients(void * data, int lenght);

	bool running;
	bool isServer;
	std::string hostName;
	std::vector<std::string> localAddresses;
	int threadID;
};

class TCPData
{
public:
	Network * mNetwork;
	unsigned int clientIndex;
};

#endif