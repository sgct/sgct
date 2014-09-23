/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/ClusterManager.h"
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>

sgct_core::ClusterManager * sgct_core::ClusterManager::mInstance = NULL;

sgct_core::ClusterManager::ClusterManager(void)
{
	masterIndex = -1;
	mThisNodeId = -1;
	validCluster = false;
	mFirmFrameLockSync = false;
	mUseASCIIForExternalControl = true;

	mUser = new SGCTUser();
	mTrackingManager = new sgct::SGCTTrackingManager();

	mSceneTransform = glm::mat4(1.0f);
	mSceneScale = glm::mat4(1.0f);
	mSceneTranslate = glm::mat4(1.0f);
	mSceneRotation = glm::mat4(1.0f);

	mMeshImpl = BUFFER_OBJECTS; //default
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
sgct_core::SGCTNode * sgct_core::ClusterManager::getNodePtr(std::size_t index)
{
	return (index < nodes.size()) ? &nodes[index] : NULL;
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
	Set if external control should use ASCII (Telnet) or raw binary parsing.
*/
void sgct_core::ClusterManager::setUseASCIIForExternalControl(bool useASCII)
{
	mUseASCIIForExternalControl = useASCII;
}

/*!
	Get if external control is using ASCII (Telnet) or raw binary parsing.
*/
bool sgct_core::ClusterManager::getUseASCIIForExternalControl()
{
	return mUseASCIIForExternalControl;
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

/*!
		\returns the dns, name or ip of the master in the cluster (depends on what's been set in the XML config)
*/
std::string * sgct_core::ClusterManager::getMasterAddress()
{
	return &mMasterAddress;
}

/*!
	\param the dns, ip or name of the master in the cluster
*/
void sgct_core::ClusterManager::setMasterAddress(std::string address)
{
	std::transform(address.begin(), address.end(), address.begin(), ::tolower);
	mMasterAddress.assign(address);
}

/*!
\returns the external control port number if it's set or specified in the XML configuration
*/
std::string sgct_core::ClusterManager::getExternalControlPort()
{
	return mExternalControlPort;
}

/*!
\param the external control port number
*/
void sgct_core::ClusterManager::setExternalControlPort(std::string port)
{
	mExternalControlPort.assign(port);
}
