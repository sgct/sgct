/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/ClusterManager.h>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>

sgct_core::ClusterManager * sgct_core::ClusterManager::mInstance = NULL;

sgct_core::ClusterManager::ClusterManager(void)
{
	masterIndex = -1;
	mThisNodeId = -1;
	validCluster = false;
	mFirmFrameLockSync = false;
	mIgnoreSync = false;
	mUseASCIIForExternalControl = true;

	SGCTUser * defaultUser = new SGCTUser("default");
	mUsers.push_back(defaultUser);
	mTrackingManager = new sgct::SGCTTrackingManager();

	mSceneTransform = glm::mat4(1.0f);
	mSceneScale = glm::mat4(1.0f);
	mSceneTranslate = glm::mat4(1.0f);
	mSceneRotation = glm::mat4(1.0f);

	mMeshImpl = BUFFER_OBJECTS; //default
	mNetMode = NetworkManager::Remote;
}

sgct_core::ClusterManager::~ClusterManager()
{
	nodes.clear();
	
	for (std::size_t i=0; i < mUsers.size(); i++)
	{
		delete mUsers[i];
		mUsers[i] = NULL;
	}
	mUsers.clear();

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
	Add an user ptr. The cluster manager will deallocate the user upon destruction.
*/
void sgct_core::ClusterManager::addUserPtr(sgct_core::SGCTUser * userPtr)
{
	mUsers.push_back( userPtr );
}

/*!
	Get a pointer to a specific node.

	\param index the index to a node in the vector
	\returns the pointer to the requested node or NULL if not found
*/
sgct_core::SGCTNode * sgct_core::ClusterManager::getNodePtr(std::size_t index)
{
	return (index < nodes.size()) ? &nodes[index] : NULL;
}

/*!
	Get a pointer to a specific node.

	\param name of the node to search for
	\returns the pointer to the requested node or NULL if not found
*/
sgct_core::SGCTNode * sgct_core::ClusterManager::getNodePtr(std::string name)
{
	for (std::size_t i = 0; i < nodes.size(); i++)
	{
		if (nodes[i].getName().compare(name) == 0)
			return &nodes[i];
	}

	//if not found
	return NULL;
}

/*!
	\returns a pointer to the node that this application is running on
*/
sgct_core::SGCTNode * sgct_core::ClusterManager::getThisNodePtr()
{
	return mThisNodeId < 0 ? NULL : &nodes[mThisNodeId];
}

/*!
\returns the pointer to the default user
*/
sgct_core::SGCTUser * sgct_core::ClusterManager::getDefaultUserPtr()
{
	return mUsers[0];
}

/*!
\returns the pointer to a named user. NULL is returned if no user is found.
*/
sgct_core::SGCTUser * sgct_core::ClusterManager::getUserPtr(std::string name)
{
	for (std::size_t i=0; i < mUsers.size(); i++)
	{
		if (mUsers[i]->getName().compare(name) == 0)
			return mUsers[i];
	}

	//if not found
	return NULL;
}

/*!
\returns the pointer to the tracked user. NULL is returned if no user is tracked.
*/
sgct_core::SGCTUser * sgct_core::ClusterManager::getTrackedUserPtr()
{
	for (std::size_t i = 0; i < mUsers.size(); i++)
	{
		if (mUsers[i]->isTracked())
			return mUsers[i];
	}
	
	//no tracking
	return NULL;
}

/*!
\returns the current network mode
*/
sgct_core::NetworkManager::NetworkMode sgct_core::ClusterManager::getNetworkMode()
{
	return mNetMode;
}

/*!
Sets the current network mode
*/
void sgct_core::ClusterManager::setNetworkMode(sgct_core::NetworkManager::NetworkMode nm)
{
	mNetMode = nm;
}

/*!
Set the scene transform.
*/
void sgct_core::ClusterManager::setSceneTransform(glm::mat4 mat)
{
	mSceneTransform = mat;
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
Set the scene rotation. This is set using the XML config file for easier transitions between different hardware setups.
*/
void sgct_core::ClusterManager::setSceneRotation(glm::mat4 mat)
{
	mSceneRotation = mat;
	calculateSceneTransform();
}

/*!
Get if software sync between nodes is disabled
*/
bool sgct_core::ClusterManager::getIgnoreSync()
{
	return mIgnoreSync;
}

/*!
Set if software sync between nodes should be ignored
*/
void sgct_core::ClusterManager::setUseIgnoreSync(bool state)
{
	mIgnoreSync = state;
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
