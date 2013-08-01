/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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
	mFirmFrameLockSync = false;

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

/*!
	Add a cluster node to the manager's vector.
*/
void sgct_core::ClusterManager::addNode(sgct_core::SGCTNode node)
{
	nodes.push_back(node);
}

/*!
	Get a pointer to a specific node. Use \link getNumberOfNodes \endlink to prevent out of bounds error.

	\param index the index to a node in the vector
	\returns the pointer to the requested node
*/
sgct_core::SGCTNode * sgct_core::ClusterManager::getNodePtr(unsigned int index)
{
	return &nodes[index];
}

/*!
	\returns a pointer to the node that this application is running on
*/
sgct_core::SGCTNode * sgct_core::ClusterManager::getThisNodePtr()
{
	return mThisNodeId < 0 ? NULL : &nodes[mThisNodeId];
}

/*!
	Set the scene offset/translation. This is set using the XML config file for easier transitions between different hardware setups.
*/
void sgct_core::ClusterManager::setSceneOffset(glm::vec3 offset)
{
	mSceneTranslate = glm::translate( glm::mat4(1.0f), offset);
	calculateSceneTransform();
}

/*!
	Set the scene rotation. This is set using the XML config file for easier transitions between different hardware setups.
*/
void sgct_core::ClusterManager::setSceneRotation(float yaw, float pitch, float roll)
{
	mSceneRotation = glm::yawPitchRoll(yaw, pitch, roll);
	calculateSceneTransform();
}

/*!
	Set the scene scale. This is set using the XML config file for easier transitions between different hardware setups.
*/
void sgct_core::ClusterManager::setSceneScale(float scale)
{
	mSceneScale = glm::scale( glm::mat4(1.0f), glm::vec3(scale) );
	calculateSceneTransform();
}

/*!
Updates the scene transform. Calculates the transform matrix using: SceneTransform = Rotation * Offset * Scale
*/
void sgct_core::ClusterManager::calculateSceneTransform()
{
	mSceneTransform = mSceneRotation * mSceneTranslate * mSceneScale;
}
