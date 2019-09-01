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
#include <glm/glm.hpp>
#include <memory>
#include <string>


/**
 * \namespace sgct_core
 * \brief SGCT core namespace is used internally within sgct.
 */
namespace sgct_core {

class SGCTUser;

/**
 * The ClusterManager manages all nodes and cluster settings. This class is a static
 * singleton and is accessed using it's instance.
 */
class ClusterManager {
public:
    /// Get the ClusterManager instance */
    static ClusterManager* instance();

    /// Destroy the ClusterManager */
    static void destroy();

    /// Different modes for warping/edge blending meshes 
    enum class MeshImplementation {
        BufferObjects = 0,
        DisplayList
    };

    /// Add a cluster node to the manager's vector.
    void addNode(SGCTNode node);

    /// Add a user ptr.
    void addUser(std::unique_ptr<SGCTUser> userPtr);

    /**
     * Get a pointer to a specific node.
     *
     * \param index the index to a node in the vector
     *
     * \return the pointer to the requested node or nullptr if not found
     */
    SGCTNode* getNodePtr(size_t index);

    /**
     * Get a pointer to a specific node.
     *
     * \param name of the node to search for
     *
     * \return the pointer to the requested node or NULL if not found
     */
    SGCTNode* getNodePtr(const std::string& name);

    /// \return a pointer to the node that this application is running on
    SGCTNode* getThisNode();

    /// \return the pointer to the default user
    SGCTUser* getDefaultUser();

    /// \return the pointer to a named user. nullptr is returned if no user is found.
    SGCTUser* getUserPtr(const std::string& name);

    /// \return the pointer to the tracked user. Returns nullptr if no user is tracked.
    SGCTUser* getTrackedUserPtr();

    /// \return the current network mode
    NetworkManager::NetworkMode getNetworkMode() const;

    /// Sets the current network mode
    void setNetworkMode(NetworkManager::NetworkMode nm);

    /// \return the number of nodes in the cluster
    size_t getNumberOfNodes() const;
    
    /// \return the scene transform specified in the configuration file
    const glm::mat4& getSceneTransform() const;

    /**
     *  Don't set this, this is done automatically using from the Network Manager which
     * compares IP addresses from this computer to the XML config file.
     *
     * \param the index to the node where this application is running on
     */
    void setThisNodeId(int id);

    /// \returns the id to the node which runs this application
    int getThisNodeId() const;

    /**
     * \returns the dns, name or IP of the master in the cluster (depends on what's been
     * set in the XML config)
     */
    const std::string& getMasterAddress() const;

    /// \param the DNS, IP or name of the master in the cluster
    void setMasterAddress(std::string address);

    /// \return state of the firm frame lock lock sync
    bool getFirmFrameLockSyncStatus() const;

    /// \param the state of the firm frame lock sync
    void setFirmFrameLockSyncStatus(bool state);

    /**
     * \return the external control port number if it's set or specified in the XML
     * configuration
     */
    const std::string& getExternalControlPort() const;

    /// \param the external control port number
    void setExternalControlPort(std::string port);

    /// Set if external control should use ASCII (Telnet) or raw binary parsing.
    void setUseASCIIForExternalControl(bool useASCII);

    /// Get if external control is using ASCII (Telnet) or raw binary parsing.
    bool getUseASCIIForExternalControl() const;

    /// Set if software sync between nodes should be ignored
    void setUseIgnoreSync(bool state);

    /// Get if software sync between nodes is disabled
    bool getIgnoreSync() const;

    /// Set the scene transform.
    void setSceneTransform(glm::mat4 mat);

    /**
     * Set the scene offset/translation. This is set using the XML config file for easier
     * transitions between different hardware setups.
     */
    void setSceneOffset(glm::vec3 offset);

    /**
     * Set the scene rotation. This is set using the XML config file for easier
     * transitions between different hardware setups.
     */
    void setSceneRotation(float yaw, float pitch, float roll);

    /**
     * Set the scene rotation. This is set using the XML config file for easier
     * transitions between different hardware setups.
     */
    void setSceneRotation(glm::mat4 mat);
    
    /**
     * Set the scene scale. This is set using the XML config file for easier transitions
     * between different hardware setups.
     */
    void setSceneScale(float scale);

    /**
     * Sets the rendering hint for SGCT's internal draw functions (must be done before
     * OpenGL init)
     */
    void setMeshImplementation(MeshImplementation impl);

    /// \returns the mesh implementation
    MeshImplementation getMeshImplementation() const;

    /// \returns the pointer to the tracking manager
    sgct::SGCTTrackingManager& getTrackingManager();

private:
    ClusterManager();
    ~ClusterManager() = default;

    /**
     * Updates the scene transform. Calculates the transform matrix using:
     * SceneTransform = Rotation * Offset * Scale
     */
    void calculateSceneTransform();

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

    std::vector<std::unique_ptr<SGCTUser>> mUsers;
    sgct::SGCTTrackingManager mTrackingManager;

    glm::mat4 mSceneTransform = glm::mat4(1.f);
    glm::mat4 mSceneScale = glm::mat4(1.f);
    glm::mat4 mSceneTranslate = glm::mat4(1.f);
    glm::mat4 mSceneRotation = glm::mat4(1.f);
    MeshImplementation mMeshImpl = MeshImplementation::BufferObjects;
    NetworkManager::NetworkMode mNetMode = NetworkManager::NetworkMode::Remote;
};

} // namespace sgct_core

#endif // __SGCT__CLUSTER_MANAGER__H__
