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
\brief simple graphics cluster toolkit core namespace.
This namespace is used internally within sgct.
*/
namespace sgct_core
{

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
	std::size_t getNumberOfNodes() const { return nodes.size(); }
	SGCTNode * getNodePtr(unsigned int index);
	SGCTNode * getThisNodePtr();
	SGCTUser * getUserPtr() { return mUser; }

	const glm::mat4 & getSceneTransform() { return mSceneTransform; }
	void setThisNodeId(int id) { mThisNodeId = id; }
	int getThisNodeId() { return mThisNodeId; }

	std::string * getMasterIp() { return &masterIp; }
	void setMasterIp(std::string ip) { masterIp.assign(ip); }

	std::string * getExternalControlPort() { return &mExternalControlPort; }
	void setExternalControlPort(std::string & port) { mExternalControlPort.assign(port); }

	void setSceneOffset(glm::vec3 offset);
	void setSceneRotation(float yaw, float pitch, float roll);
	void setSceneScale(float scale);

	void setMeshImplementation( MeshImplementation impl ) { mMeshImpl = impl; }
	inline MeshImplementation getMeshImplementation() { return mMeshImpl; }

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
