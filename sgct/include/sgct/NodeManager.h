/* NodeManager.h

© 2012 Miroslav Andel

*/

#ifndef _NODE_MANAGER
#define _NODE_MANAGER

#include "SGCTNode.h"
#include <vector>
#include <string>

namespace core_sgct
{

class NodeManager
{
public:
	/*! Get the NodeManager instance */
	static NodeManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new NodeManager();
		}

		return mInstance;
	}

	/*! Destroy the NodeManager */
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
	NodeManager(void);
	~NodeManager(void);

	// Don't implement these, should give compile warning if used
	NodeManager( const NodeManager & nm );
	const NodeManager & operator=(const NodeManager & nm );

private:
	static NodeManager * mInstance;

	std::vector<SGCTNode> nodes;
	int masterIndex;
	int mThisNodeId;
	bool validCluster;
	std::string masterIp;
	std::string mExternalControlPort;
};
}

#endif