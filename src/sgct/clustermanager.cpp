/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/clustermanager.h>

#include <sgct/config.h>
#include <sgct/logger.h>
#include <sgct/settings.h>
#include <algorithm>

namespace sgct::core {

ClusterManager* ClusterManager::_instance = nullptr;

ClusterManager& ClusterManager::instance() {
    if (!_instance) {
        _instance = new ClusterManager;
    }
    return *_instance;
}

void ClusterManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

ClusterManager::ClusterManager() {
    _users.push_back(User("default"));
}

void ClusterManager::applyCluster(const config::Cluster& cluster) {
    _masterAddress = cluster.masterAddress;
    if (cluster.debugLog && *cluster.debugLog) {
        Logger::instance().setNotifyLevel(Logger::Level::Debug);
    }
    if (cluster.externalControlPort) {
        setExternalControlPort(*cluster.externalControlPort);
    }
    if (cluster.firmSync) {
        setFirmFrameLockSyncStatus(*cluster.firmSync);
    }
    if (cluster.scene) {
        const glm::mat4 sceneTranslate = cluster.scene->offset ?
            glm::translate(glm::mat4(1.f), *cluster.scene->offset) :
            glm::mat4(1.f);

        const glm::mat4 sceneRotation = cluster.scene->orientation ?
            glm::mat4_cast(*cluster.scene->orientation) :
            glm::mat4(1.f);

        const glm::mat4 sceneScale = cluster.scene->scale ?
            glm::scale(glm::mat4(1.f), glm::vec3(*cluster.scene->scale)) :
            glm::mat4(1.f);

        _sceneTransform = sceneRotation * sceneTranslate * sceneScale;
    }
    // The users must be handled before the nodes due to the nodes depending on the users
    for (const config::User& user : cluster.users) {
        User* usrPtr;
        if (user.name) {
            User usr(*user.name);
            addUser(std::move(usr));
            usrPtr = &_users.back();
            Logger::Info("Adding user '%s'", user.name->c_str());
        }
        else {
            usrPtr = &defaultUser();
        }

        if (user.eyeSeparation) {
            usrPtr->setEyeSeparation(*user.eyeSeparation);
        }
        if (user.position) {
            usrPtr->setPos(*user.position);
        }
        if (user.transformation) {
            usrPtr->setTransform(*user.transformation);
        }
        if (user.tracking) {
            usrPtr->setHeadTracker(user.tracking->tracker, user.tracking->device);
        }
    }
    for (const config::Node& node : cluster.nodes) {
        Node n;
        n.applyNode(node);
        addNode(std::move(n));
    }
    if (cluster.settings) {
        Settings::instance().applySettings(*cluster.settings);
    }
    if (cluster.capture) {
        Settings::instance().applyCapture(*cluster.capture);
    }
}

void ClusterManager::addNode(Node node) {
    _nodes.push_back(std::move(node));
}

void ClusterManager::addUser(User user) {
    _users.push_back(std::move(user));
}

const Node& ClusterManager::node(int index) const {
    return _nodes[index];
}

Node& ClusterManager::thisNode() {
    return _nodes[_thisNodeId];
}

const Node& ClusterManager::thisNode() const {
    return _nodes[_thisNodeId];
}

User& ClusterManager::defaultUser() {
    // This object is guaranteed to exist as we add it in the constructor and it is not
    // possible to clear the _users list
    return _users[0];
}

User* ClusterManager::user(const std::string& name) {
    auto it = std::find_if(
        _users.begin(),
        _users.end(),
        [&name](const User& user) { return user.name() == name; }
    );
    return it != _users.end() ? &*it : nullptr;
}

User* ClusterManager::trackedUser() {
    auto it = std::find_if(
        _users.begin(),
        _users.end(),
        [](const User& u) { return u.isTracked(); }
    );
    return it != _users.end() ? &*it : nullptr;
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

void ClusterManager::setThisNodeId(int id) {
    _thisNodeId = id;
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

} // namespace sgct::core
