/* ClusterManager.h

© 2012 Miroslav Andel

*/

#ifndef _CLUSTER_MANAGER
#define _CLUSTER_MANAGER

#include "SGCTNode.h"
#include <vector>
#include <string>

namespace core_sgct
{

class ClusterManager
{
public:
	/*! Get the ClusterManager instance */
	static ClusterManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new ClusterManager();
		}

		return mInstance;
	}

	/*! Destroy the ClusterManager */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void addNode(SGCTNode node);
	unsigned int getNumberOfNodes() const { return nodes.size(); }
	SGCTNode * getNodePtr(unsigned int index);
	SGCTNode * getThisNodePtr();
	void setThisNodeId(int id) { mThisNodeId = id; }
	int getThisNodeId() { return mThisNodeId; }

	std::string * getMasterIp() { return &masterIp; }
	void setMasterIp(std::string & ip) { masterIp.assign(ip); }

	std::string * getExternalControlPort() { return &mExternalControlPort; }
	void setExternalControlPort(std::string & port) { mExternalControlPort.assign(port); }

private:
	ClusterManager(void);
	~ClusterManager(void);

	// Don't implement these, should give compile warning if used
	ClusterManager( const ClusterManager & nm );
	const ClusterManager & operator=(const ClusterManager & nm );

private:
	static ClusterManager * mInstance;

	std::vector<SGCTNode> nodes;
	int masterIndex;
	int mThisNodeId;
	bool validCluster;
	std::string masterIp;
	std::string mExternalControlPort;
};
}

#endif