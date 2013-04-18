/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _CLUSTER_MANAGER
#define _CLUSTER_MANAGER

#include "SGCTNode.h"
#include "SGCTUser.h"
#include "SGCTTrackingManager.h"
#include <string>

/*! \namespace sgct_core
\brief Simple Graphics Cluster Toolkit core namespace.
This namespace is used internally within sgct.
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

	/*!
		Different modes for warping/edge blending meshes 
	*/
	enum MeshImplementation { VBO_INDEX=0, VBO_ARRAY, DISPLAY_LIST };
	/*!
		Different stereo modes used for rendering
	*/
	enum StereoMode { NoStereo = 0, Active, Anaglyph_Red_Cyan, Anaglyph_Amber_Blue, Anaglyph_Red_Cyan_Wimmer, Checkerboard, Checkerboard_Inverted, Vertical_Interlaced, Vertical_Interlaced_Inverted };

	void addNode(SGCTNode node);

	SGCTNode * getNodePtr(unsigned int index);
	SGCTNode * getThisNodePtr();

	/*!
		\returns the pointer to the user (where all the projections are calculated from)
	*/
	SGCTUser * getUserPtr() { return mUser; }

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

	/*!
		\returns the ip address to the master in the cluster
	*/
	std::string * getMasterIp() { return &masterIp; }

	/*!
		\param the ip address to the master
	*/
	void setMasterIp(std::string ip) { masterIp.assign(ip); }

	/*!
		\returns state of the firm frame lock lock sync
	*/
	bool getFirmFrameLockSyncStatus() { return mFirmFrameLockSync; }

	/*!
		\param the state of the firm frame lock sync
	*/
	void setFirmFrameLockSyncStatus( bool state ) { mFirmFrameLockSync = state; }

	/*!
		\returns the external control port number if it's set or specified in the XML configuration
	*/
	std::string * getExternalControlPort() { return &mExternalControlPort; }

	/*!
		\param the external control port number
	*/
	void setExternalControlPort(std::string & port) { mExternalControlPort.assign(port); }

	void setSceneOffset(glm::vec3 offset);
	void setSceneRotation(float yaw, float pitch, float roll);
	void setSceneScale(float scale);

	/*!
		Sets the warping and blending mesh implemtation
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
	std::string masterIp;
	std::string mExternalControlPort;

	SGCTUser * mUser;
	sgct::SGCTTrackingManager * mTrackingManager;

	glm::mat4 mSceneTransform;
	glm::mat4 mSceneScale;
	glm::mat4 mSceneTranslate;
	glm::mat4 mSceneRotation;
	MeshImplementation mMeshImpl;
};
}

#endif
