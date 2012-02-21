#include "../include/sgct/NodeManager.h"

core_sgct::NodeManager * core_sgct::NodeManager::mInstance = NULL;

core_sgct::NodeManager::NodeManager(void)
{
	masterIndex = -1;
	mThisNodeId = -1;
	validCluster = false;
}

core_sgct::NodeManager::~NodeManager()
{
	nodes.clear();
}

void core_sgct::NodeManager::addNode(core_sgct::SGCTNode node)
{
	nodes.push_back(node);
}

core_sgct::SGCTNode * core_sgct::NodeManager::getNodePtr(unsigned int index)
{
	return &nodes[index];
}

core_sgct::SGCTNode * core_sgct::NodeManager::getThisNodePtr()
{
	return &nodes[mThisNodeId];
}