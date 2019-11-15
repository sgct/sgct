/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CLUSTER_MANAGER__H__
#define __SGCT__CLUSTER_MANAGER__H__

#include <sgct/node.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace sgct::config { struct Cluster; }

namespace sgct::core {

/**
 * The ClusterManager manages all nodes and cluster settings. This class is a static
 * singleton and is accessed using it's instance.
 */
class ClusterManager {
public:
    static ClusterManager& instance();
    static void destroy();

    void applyCluster(const config::Cluster& cluster);

    /// Add a cluster node to the manager's vector.
    void addNode(Node node);

    /// Add a new user.
    void addUser(User user);

    /**
     * Get a pointer to a specific node. Please observe that the address of this object
     * might change between frames and should not be kept around for long.
     *
     * \param int the index to a node in the vector
     * \return the pointer to the requested node. This pointer is
     *         not guaranteed to be stable between function calls
     */
    const Node& getNode(int index) const;

    /**
     * Get the current node. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return a reference to the node that this application is running on
     */
    Node& getThisNode();

    /**
     * Get the current node. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return a reference to the node that this application is running on
     */
    const Node& getThisNode() const;

    /**
     * Get the default user. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return the pointer to the default user
     */
    User& getDefaultUser();

    /**
     * Get the user with the specific name. Please observe that the address of this object
     * might change between frames and should not be stored.
     *
     * \return the pointer to a named user. nullptr is returned if no user is found.
     */
    User* getUser(const std::string& name);

    /**
     * Get the tracked user. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return the pointer to the tracked user. Returns nullptr if no user is tracked.
     */
    User* getTrackedUser();

    /// \return the number of nodes in the cluster
    int getNumberOfNodes() const;
    
    /// \return the scene transform specified in the configuration file
    const glm::mat4& getSceneTransform() const;

    /**
     * Don't set this, this is done automatically using from the Network Manager which
     * compares IP addresses from this computer to the XML config file.
     *
     * \param the index to the node where this application is running on
     */
    void setThisNodeId(int id);

    /// \return the id to the node which runs this application
    int getThisNodeId() const;

    /// \return the dns, name or IP of the master in the cluster
    const std::string& getMasterAddress() const;

    /// \return state of the firm frame lock lock sync
    bool getFirmFrameLockSyncStatus() const;

    /// \param the state of the firm frame lock sync
    void setFirmFrameLockSyncStatus(bool state);

    /// \return the external control port number
    int getExternalControlPort() const;

    /// \param the external control port number
    void setExternalControlPort(int port);

    /// Set if software sync between nodes should be ignored
    void setUseIgnoreSync(bool state);

    /// Get if software sync between nodes is disabled
    bool getIgnoreSync() const;

private:
    ClusterManager();

    static ClusterManager* _instance;

    int _thisNodeId = -1;
    bool _firmFrameLockSync = false;
    bool _ignoreSync = false;
    std::string _masterAddress;
    int _externalControlPort = 0;

    std::vector<Node> _nodes;
    std::vector<User> _users;
    glm::mat4 _sceneTransform = glm::mat4(1.f);
};

} // namespace sgct::core

#endif // __SGCT__CLUSTER_MANAGER__H__
