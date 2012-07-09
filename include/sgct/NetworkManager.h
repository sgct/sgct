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

#ifndef _NETWORK_MANAGER_H_
#define _NETWORK_MANAGER_H_

#include "SGCTNetwork.h"
#include <vector>
#include <string>

typedef void * GLFWmutex;
typedef void * GLFWcond;

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
	SGCTNetwork * getExternalControlPtr();

	unsigned int getConnectionsCount() { return mNumberOfConnections; }

private:
	bool addConnection(const std::string port, const std::string ip, int serverType = SGCTNetwork::SyncServer);
	void initAPI();
	void getHostInfo();
	void updateConnectionStatus(int index);
	void setAllNodesConnected();

public:
	enum ManagerMode { NotLocal = 0, LocalServer, LocalClient };
	static GLFWmutex gMutex;
	static GLFWmutex gSyncMutex;
	static GLFWcond gCond;

private:
	std::vector<SGCTNetwork*> mNetworkConnections;

	std::string hostName; //stores this computers hostname
	std::vector<std::string> localAddresses; //stors this computers ip addresses

	bool mIsServer;
	bool mIsRunning;
	bool mIsExternalControlPresent;
	bool mAllNodesConnected;
	int mMode;
	unsigned int mNumberOfConnections;
};

}

#endif
