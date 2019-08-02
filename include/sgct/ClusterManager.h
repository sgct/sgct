/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__CLUSTER_MANAGER__H__
#define __SGCT__CLUSTER_MANAGER__H__

#include <sgct/SGCTNode.h>
#include <sgct/SGCTTrackingManager.h>
#include <sgct/NetworkManager.h>
#include <string>

/*! \namespace sgct_core
\brief SGCT core namespace is used internally within sgct.
*/
namespace sgct_core {

class SGCTUser;

/*!
The ClusterManager manages all nodes and cluster settings.
This class is a static singleton and is accessed using it's instance.
*/
class ClusterManager {
public:
    /*! Get the ClusterManager instance */
    static ClusterManager* instance();

    /*! Destroy the ClusterManager */
    static void destroy();

    /*!
        Different modes for warping/edge blending meshes 
    */
    enum MeshImplementation {
        BUFFER_OBJECTS = 0,
        DISPLAY_LIST
    };

    void addNode(SGCTNode node);
    void addUserPtr(SGCTUser* userPtr);

    SGCTNode* getNodePtr(size_t index);
    SGCTNode* getNodePtr(const std::string& name);
    SGCTNode* getThisNodePtr();
    SGCTUser* getDefaultUserPtr();
    SGCTUser* getUserPtr(const std::string& name);
    SGCTUser* getTrackedUserPtr();
    NetworkManager::NetworkMode getNetworkMode() const;
    void setNetworkMode(NetworkManager::NetworkMode nm);

    /*!
        \returns the number of nodes in the cluster
    */
    std::size_t getNumberOfNodes() const;
    
    /*!
        \returns the scene transform specified in the configuration file
    */
    const glm::mat4& getSceneTransform() const;

    /*!
        Don't set this, this is done automatically using from the Network Manager which compares
        ip addresses from this computer to the XML config file.
        
        \param the index to the node where this application is running on
    */
    void setThisNodeId(int id);

    /*!
        \returns the id to the node which runs this application
    */
    int getThisNodeId() const;

    const std::string& getMasterAddress() const;
    void setMasterAddress(std::string address);

    /*!
        \returns state of the firm frame lock lock sync
    */
    bool getFirmFrameLockSyncStatus() const;

    /*!
        \param the state of the firm frame lock sync
    */
    void setFirmFrameLockSyncStatus(bool state);

    const std::string& getExternalControlPort() const;
    void setExternalControlPort(std::string port);

    void setUseASCIIForExternalControl(bool useASCII);
    bool getUseASCIIForExternalControl() const;

    void setUseIgnoreSync(bool state);
    bool getIgnoreSync() const;

    void setSceneTransform(glm::mat4 mat);
    void setSceneOffset(glm::vec3 offset);
    void setSceneRotation(float yaw, float pitch, float roll);
    void setSceneRotation(glm::mat4 mat);
    void setSceneScale(float scale);

    /*!
        Sets the rendering hint for SGCT's internal draw functions (must be done before opengl init)
    */
    void setMeshImplementation(MeshImplementation impl);

    /*!
        \returns the mesh implementation
    */
    MeshImplementation getMeshImplementation() const;

    /*!
        \returns the pointer to the tracking manager
    */
    sgct::SGCTTrackingManager* getTrackingManagerPtr();

private:
    ClusterManager();
    ~ClusterManager();

    void calculateSceneTransform();

private:
    static ClusterManager* mInstance;

    std::vector<SGCTNode> nodes;

    int masterIndex = -1;
    int mThisNodeId = -1;
    bool validCluster = false;
    bool mFirmFrameLockSync = false;
    bool mIgnoreSync = false;
    std::string mMasterAddress;
    std::string mExternalControlPort;
    bool mUseASCIIForExternalControl = true;

    std::vector<SGCTUser*> mUsers;
    sgct::SGCTTrackingManager mTrackingManager;

    glm::mat4 mSceneTransform = glm::mat4(1.f);
    glm::mat4 mSceneScale = glm::mat4(1.f);
    glm::mat4 mSceneTranslate = glm::mat4(1.f);
    glm::mat4 mSceneRotation = glm::mat4(1.f);
    MeshImplementation mMeshImpl = BUFFER_OBJECTS;
    NetworkManager::NetworkMode mNetMode = NetworkManager::Remote;
};

} // namespace sgct_core

#endif // __SGCT__CLUSTER_MANAGER__H__
