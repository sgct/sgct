/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _CLUSTER_MANAGER
#define _CLUSTER_MANAGER

#include "SGCTNode.h"
#include "SGCTUser.h"
#include "SGCTTrackingManager.h"
#include "NetworkManager.h"
#include <string>

/*! \namespace sgct_core
\brief SGCT core namespace is used internally within sgct.
*/
namespace sgct_core
{

/*!
The ClusterManager manages all nodes and cluster settings.
This class is a static singleton and is accessed using it's instance.
*/
class ClusterManager
{
public:
	/*! Get the ClusterManager instance */
	static ClusterManager * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new ClusterManager();
		}

		return mInstance;
	}

	/*! Destroy the ClusterManager */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	/*!
		Different modes for warping/edge blending meshes 
	*/
	enum MeshImplementation { BUFFER_OBJECTS=0, DISPLAY_LIST };

	void addNode(SGCTNode node);
	void addUserPtr(SGCTUser * userPtr);

	SGCTNode * getNodePtr(std::size_t index);
	SGCTNode * getNodePtr(std::string name);
	SGCTNode * getThisNodePtr();
	SGCTUser * getDefaultUserPtr();
	SGCTUser * getUserPtr(std::string name);
	NetworkManager::NetworkMode getNetworkMode();
	void setNetworkMode(NetworkManager::NetworkMode nm);

	/*!
		\returns the number of nodes in the cluster
	*/
	std::size_t getNumberOfNodes() const { return nodes.size(); }
	
	/*!
		\returns the scene transform specified in the configuration file
	*/
	const glm::mat4 & getSceneTransform() { return mSceneTransform; }

	/*!
		Don't set this, this is done automatically using from the Network Manager which compares
		ip addresses from this computer to the XML config file.
		
		\param the index to the node where this application is running on
	*/
	void setThisNodeId(int id) { mThisNodeId = id; }

	/*!
		\returns the id to the node which runs this application
	*/
	int getThisNodeId() { return mThisNodeId; }

	std::string * getMasterAddress();
	void setMasterAddress(std::string address);

	/*!
		\returns state of the firm frame lock lock sync
	*/
	bool getFirmFrameLockSyncStatus() { return mFirmFrameLockSync; }

	/*!
		\param the state of the firm frame lock sync
	*/
	void setFirmFrameLockSyncStatus( bool state ) { mFirmFrameLockSync = state; }

	std::string getExternalControlPort();
	void setExternalControlPort(std::string port);

	void setUseASCIIForExternalControl(bool useASCII);
	bool getUseASCIIForExternalControl();

	void setUseIgnoreSync(bool state);
	bool getIgnoreSync();

	void setSceneTransform(glm::mat4 mat);
	void setSceneOffset(glm::vec3 offset);
	void setSceneRotation(float yaw, float pitch, float roll);
	void setSceneRotation(glm::mat4 mat);
	void setSceneScale(float scale);

	/*!
		Sets the rendering hint for SGCT's internal draw functions (must be done before opengl init)
	*/
	void setMeshImplementation( MeshImplementation impl ) { mMeshImpl = impl; }

	/*!
		\returns the mesh implementation
	*/
	inline MeshImplementation getMeshImplementation() { return mMeshImpl; }

	/*!
		\returns the pointer to the tracking manager
	*/
	inline sgct::SGCTTrackingManager * getTrackingManagerPtr() { return mTrackingManager; }

private:
	ClusterManager(void);
	~ClusterManager(void);

	// Don't implement these, should give compile warning if used
	ClusterManager( const ClusterManager & nm );
	const ClusterManager & operator=(const ClusterManager & nm );

	void calculateSceneTransform();

private:
	static ClusterManager * mInstance;

	std::vector<SGCTNode> nodes;

	int masterIndex;
	int mThisNodeId;
	bool validCluster;
	bool mFirmFrameLockSync;
	bool mIgnoreSync;
	std::string mMasterAddress;
	std::string mExternalControlPort;
	bool mUseASCIIForExternalControl;

	std::vector<SGCTUser*> mUsers;
	sgct::SGCTTrackingManager * mTrackingManager;

	glm::mat4 mSceneTransform;
	glm::mat4 mSceneScale;
	glm::mat4 mSceneTranslate;
	glm::mat4 mSceneRotation;
	MeshImplementation mMeshImpl;
	NetworkManager::NetworkMode mNetMode;
};
}

#endif
