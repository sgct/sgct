/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/clustermanager.h>

#include <sgct/config.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/node.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace sgct {

ClusterManager* ClusterManager::_instance = nullptr;

ClusterManager& ClusterManager::instance() {
    if (_instance == nullptr) {
        throw std::logic_error("Using the instance before it was created or set");
    }
    return *_instance;
}

void ClusterManager::create(const config::Cluster& cluster, int clusterID) {
    ZoneScoped;

    _instance = new ClusterManager(cluster, clusterID);
    // The users must be handled before the nodes due to the nodes depending on the users
    for (config::User u : cluster.users) {
        ZoneScopedN("Create User");

        auto usr = std::make_unique<User>(u);
        _instance->_users.push_back(std::move(usr));
    }

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        ZoneScopedN("Create Node");

        auto n = std::make_unique<Node>(
            cluster.nodes[i],
            static_cast<int>(i) == _instance->_thisNodeId
        );
        _instance->_nodes.push_back(std::move(n));
    }
}

void ClusterManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

ClusterManager::ClusterManager(const config::Cluster& cluster, int clusterID)
    : _thisNodeId(clusterID)
    , _firmFrameLockSync(cluster.firmSync.value_or(false))
    , _masterAddress(cluster.masterAddress)
{
    ZoneScoped;

    if (cluster.debugLog && *cluster.debugLog) {
        Log::instance().setNotifyLevel(Log::Level::Debug);
    }

    if (cluster.scene) {
        const glm::mat4 translate = cluster.scene->offset ?
            glm::translate(
                glm::mat4(1.f), glm::make_vec3(&cluster.scene->offset->x)
            ) : glm::mat4(1.f);

        const glm::mat4 rotation = cluster.scene->orientation ?
            glm::mat4_cast(glm::make_quat(&cluster.scene->orientation->x)) :
            glm::mat4(1.f);

        const glm::mat4 scale = cluster.scene->scale ?
            glm::scale(glm::mat4(1.f), glm::vec3(*cluster.scene->scale)) : glm::mat4(1.f);

        glm::mat4 complete = rotation * translate * scale;
        std::memcpy(&_sceneTransform, glm::value_ptr(complete), sizeof(sgct::mat4));
    }
}

const Node& ClusterManager::node(int index) const {
    return *_nodes[index];
}

Node& ClusterManager::thisNode() {
    return *_nodes[_thisNodeId];
}

const Node& ClusterManager::thisNode() const {
    return *_nodes[_thisNodeId];
}

User& ClusterManager::defaultUser() {
    // This object is guaranteed to exist as we add it in the constructor and it is not
    // possible to clear the _users list
    return *_users[0];
}

User* ClusterManager::user(std::string_view name) {
    const auto it = std::find_if(
        _users.cbegin(),
        _users.cend(),
        [&name](const std::unique_ptr<User>& user) { return user->name() == name; }
    );
    return it != _users.cend() ? it->get() : nullptr;
}

User* ClusterManager::trackedUser() {
    const auto it = std::find_if(
        _users.cbegin(),
        _users.cend(),
        std::mem_fn(&User::isTracked)
    );
    return it != _users.cend() ? it->get() : nullptr;
}

bool ClusterManager::ignoreSync() const {
    return _ignoreSync;
}

void ClusterManager::setUseIgnoreSync(bool state) {
    _ignoreSync = state;
}

const std::string& ClusterManager::masterAddress() const {
    return _masterAddress;
}

int ClusterManager::numberOfNodes() const {
    return static_cast<int>(_nodes.size());
}

const mat4& ClusterManager::sceneTransform() const {
    return _sceneTransform;
}

int ClusterManager::thisNodeId() const {
    return _thisNodeId;
}

void ClusterManager::setFirmFrameLockSyncStatus(bool state) {
    _firmFrameLockSync = state;
}

bool ClusterManager::firmFrameLockSyncStatus() const {
    return _firmFrameLockSync;
}

} // namespace sgct
