/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CLUSTERMANAGER__H__
#define __SGCT__CLUSTERMANAGER__H__

#include <sgct/sgctexports.h>

#include <sgct/math.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sgct {

namespace config { struct Cluster; }

class Node;
class User;

/**
 * The ClusterManager manages all nodes and cluster settings. This class is a static
 * singleton and is accessed using its instance.
 */
class SGCT_EXPORT ClusterManager {
public:
    static ClusterManager& instance();
    static void create(const config::Cluster& cluster, int clusterID);
    static void destroy();

    /**
     * Get a pointer to a specific node. Please observe that the address of this object
     * might change between frames and should not be kept around for long.
     *
     * \param index The index to a node in the vector
     * \return The pointer to the requested node. This pointer is not guaranteed to be
     *         stable between function calls
     */
    const Node& node(int index) const;

    /**
     * Get the current node. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return A reference to the node that this application is running on
     */
    Node& thisNode();

    /**
     * Get the current node. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return A reference to the node that this application is running on
     */
    const Node& thisNode() const;

    /**
     * Get the default user. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return The pointer to the default user
     */
    User& defaultUser();

    /**
     * Get the user with the specific name. Please observe that the address of this object
     * might change between frames and should not be stored.
     *
     * \return The pointer to a named user. nullptr is returned if no user is found
     */
    User* user(std::string_view name);

    /**
     * Get the tracked user. Please observe that the address of this object might change
     * between frames and should not be stored.
     *
     * \return The pointer to the tracked user. Returns nullptr if no user is tracked
     */
    User* trackedUser();

    /**
     * \return the number of nodes in the cluster
     */
    int numberOfNodes() const;

    /**
     * \return the scene transform specified in the configuration file
     */
    const mat4& sceneTransform() const;

    /**
     * \return the id to the node which runs this application
     */
    int thisNodeId() const;

    /**
     * \return the DNS name or IP of the master in the cluster
     */
    const std::string& masterAddress() const;

    /**
     * \param state the state of the firm frame lock sync
     */
    void setFirmFrameLockSyncStatus(bool state);

    /**
     * \return state of the firm frame lock lock sync
     */
    bool firmFrameLockSyncStatus() const;

    /**
     * Set if software sync between nodes should be ignored.
     */
    void setUseIgnoreSync(bool state);

    /**
     * Get if software sync between nodes is disabled.
     */
    bool ignoreSync() const;

private:
    ClusterManager(const config::Cluster& cluster, int clusterID);
    ClusterManager(const ClusterManager&) = delete;
    ClusterManager(ClusterManager&&) = delete;
    ClusterManager& operator=(const ClusterManager&) = delete;
    ClusterManager& operator=(ClusterManager&&) = delete;

    ~ClusterManager() = default;

    static ClusterManager* _instance;

    const int _thisNodeId;
    bool _firmFrameLockSync;
    bool _ignoreSync = false;
    std::string _masterAddress;

    std::vector<std::unique_ptr<Node>> _nodes;
    std::vector<std::unique_ptr<User>> _users;
    mat4 _sceneTransform = mat4(1.f);
};

} // namespace sgct

#endif // __SGCT__CLUSTERMANAGER__H__
