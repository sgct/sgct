/* NodeManager.h

© 2012 Miroslav Andel

*/

#ifndef _NODE_MANAGER
#define _NODE_MANAGER

#include "SGCTNode.h"
#include <vector>

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
	bool validCluster;
};
}

#endif