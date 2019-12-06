/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/clustermanager.h>

#include <sgct/config.h>
#include <sgct/log.h>
#include <sgct/node.h>
#include <sgct/settings.h>
#include <sgct/user.h>
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
    _instance = new ClusterManager(clusterID);
    _instance->applyCluster(cluster);
}

void ClusterManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

ClusterManager::ClusterManager(int clusterID) : _thisNodeId(clusterID) {
    _users.push_back(std::make_unique<User>("default"));
}

ClusterManager::~ClusterManager() {}

void ClusterManager::applyCluster(const config::Cluster& cluster) {
    _masterAddress = cluster.masterAddress;
    if (cluster.debugLog && *cluster.debugLog) {
        Log::instance().setNotifyLevel(Log::Level::Debug);
    }
    if (cluster.externalControlPort) {
        setExternalControlPort(*cluster.externalControlPort);
    }
    if (cluster.firmSync) {
        setFirmFrameLockSyncStatus(*cluster.firmSync);
    }
    if (cluster.scene) {
        const glm::mat4 sceneTranslate = cluster.scene->offset ?
            glm::translate(glm::mat4(1.f), *cluster.scene->offset) : glm::mat4(1.f);

        const glm::mat4 sceneRotation = cluster.scene->orientation ?
            glm::mat4_cast(*cluster.scene->orientation) : glm::mat4(1.f);

        const glm::mat4 sceneScale = cluster.scene->scale ?
            glm::scale(glm::mat4(1.f), glm::vec3(*cluster.scene->scale)) : glm::mat4(1.f);

        _sceneTransform = sceneRotation * sceneTranslate * sceneScale;
    }
    // The users must be handled before the nodes due to the nodes depending on the users
    for (const config::User& u : cluster.users) {
        std::string name;
        if (u.name) {
            name = *u.name;
            std::unique_ptr<User> usr = std::make_unique<User>(*u.name);
            addUser(std::move(usr));
            Log::Info("Adding user '%s'", u.name->c_str());
        }
        else {
            name = "default";
        }
        User* usr = user(name);

        if (u.eyeSeparation) {
            usr->setEyeSeparation(*u.eyeSeparation);
        }
        if (u.position) {
            usr->setPos(*u.position);
        }
        if (u.transformation) {
            usr->setTransform(*u.transformation);
        }
        if (u.tracking) {
            usr->setHeadTracker(u.tracking->tracker, u.tracking->device);
        }
    }
    for (const config::Node& node : cluster.nodes) {
        std::unique_ptr<Node> n = std::make_unique<Node>();
        n->applyNode(node);
        addNode(std::move(n));
    }
    if (cluster.settings) {
        Settings::instance().applySettings(*cluster.settings);
    }
    if (cluster.capture) {
        Settings::instance().applyCapture(*cluster.capture);
    }
}

void ClusterManager::addNode(std::unique_ptr<Node> node) {
    _nodes.push_back(std::move(node));
}

void ClusterManager::addUser(std::unique_ptr<User> user) {
    _users.push_back(std::move(user));
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

User* ClusterManager::user(const std::string& name) {
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
        [](const std::unique_ptr<User>& u) { return u->isTracked(); }
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

int ClusterManager::externalControlPort() const {
    return _externalControlPort;
}

void ClusterManager::setExternalControlPort(int port) {
    _externalControlPort = port;
}

int ClusterManager::numberOfNodes() const {
    return static_cast<int>(_nodes.size());
}

const glm::mat4& ClusterManager::sceneTransform() const {
    return _sceneTransform;
}

int ClusterManager::thisNodeId() const {
    return _thisNodeId;
}

bool ClusterManager::firmFrameLockSyncStatus() const {
    return _firmFrameLockSync;
}

void ClusterManager::setFirmFrameLockSyncStatus(bool state) {
    _firmFrameLockSync = state;
}

} // namespace sgct
