#include "../include/sgct/ClusterManager.h"

core_sgct::ClusterManager * core_sgct::ClusterManager::mInstance = NULL;

core_sgct::ClusterManager::ClusterManager(void)
{
	masterIndex = -1;
	mThisNodeId = -1;
	validCluster = false;
	mUser = new User();
}

core_sgct::ClusterManager::~ClusterManager()
{
	nodes.clear();
	delete mUser;
	mUser = NULL;
}

void core_sgct::ClusterManager::addNode(core_sgct::SGCTNode node)
{
	nodes.push_back(node);
}

core_sgct::SGCTNode * core_sgct::ClusterManager::getNodePtr(unsigned int index)
{
	return &nodes[index];
}

core_sgct::SGCTNode * core_sgct::ClusterManager::getThisNodePtr()
{
	return mThisNodeId < 0 ? NULL : &nodes[mThisNodeId];
}