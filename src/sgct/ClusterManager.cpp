/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/ClusterManager.h"
#include <glm/gtx/euler_angles.hpp>

sgct_core::ClusterManager * sgct_core::ClusterManager::mInstance = NULL;

sgct_core::ClusterManager::ClusterManager(void)
{
	masterIndex = -1;
	mThisNodeId = -1;
	validCluster = false;

	mUser = new SGCTUser();
	mTrackingManager = new sgct::SGCTTrackingManager();

	mSceneTransform = glm::mat4(1.0f);
	mSceneScale = glm::mat4(1.0f);
	mSceneTranslate = glm::mat4(1.0f);
	mSceneRotation = glm::mat4(1.0f);

	mMeshImpl = DISPLAY_LIST; //default
}

sgct_core::ClusterManager::~ClusterManager()
{
	nodes.clear();
	
	delete mUser;
	mUser = NULL;

	delete mTrackingManager;
	mTrackingManager = NULL;
}

void sgct_core::ClusterManager::addNode(sgct_core::SGCTNode node)
{
	nodes.push_back(node);
}

sgct_core::SGCTNode * sgct_core::ClusterManager::getNodePtr(unsigned int index)
{
	return &nodes[index];
}

sgct_core::SGCTNode * sgct_core::ClusterManager::getThisNodePtr()
{
	return mThisNodeId < 0 ? NULL : &nodes[mThisNodeId];
}

void sgct_core::ClusterManager::setSceneOffset(glm::vec3 offset)
{
	mSceneTranslate = glm::translate( glm::mat4(1.0f), offset);
	calculateSceneTransform();
}

void sgct_core::ClusterManager::setSceneRotation(float yaw, float pitch, float roll)
{
	mSceneRotation = glm::yawPitchRoll(yaw, pitch, roll);
	calculateSceneTransform();
}

void sgct_core::ClusterManager::setSceneScale(float scale)
{
	mSceneScale = glm::scale( glm::mat4(1.0f), glm::vec3(scale) );
	calculateSceneTransform();
}

/*!
Updates the scene transform.
*/
void sgct_core::ClusterManager::calculateSceneTransform()
{
	mSceneTransform = mSceneRotation * mSceneTranslate * mSceneScale;
}
