/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/clustermanager.h>

#include <sgct/user.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct::core {

ClusterManager* ClusterManager::_instance = nullptr;

ClusterManager* ClusterManager::instance()  {
    if (!_instance) {
        _instance = new ClusterManager();
    }

    return _instance;
}

void ClusterManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

ClusterManager::ClusterManager() {
    _users.push_back(std::make_unique<User>("default"));
}

void ClusterManager::addNode(std::unique_ptr<Node> node) {
    _nodes.push_back(std::move(node));
}

void ClusterManager::addUser(std::unique_ptr<User> userPtr) {
    _users.push_back(std::move(userPtr));
}

Node* ClusterManager::getNode(size_t index) {
    return (index < _nodes.size()) ? _nodes[index].get() : nullptr;
}

Node* ClusterManager::getNode(const std::string& name) {
    auto it = std::find_if(
        _nodes.begin(),
        _nodes.end(),
        [&name](const std::unique_ptr<Node>& n) { return n->getName() == name; }
    );
    if (it != _nodes.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

Node* ClusterManager::getThisNode() {
    return _thisNodeId < 0 ? nullptr : _nodes[_thisNodeId].get();
}

User& ClusterManager::getDefaultUser() {
    // This object is guaranteed to exist as we add it in the constructor and it is not
    // possible to clear the mUsers list
    return *_users[0];
}

User* ClusterManager::getUser(const std::string& name) {
    auto it = std::find_if(
        _users.begin(),
        _users.end(),
        [&name](const std::unique_ptr<User>& user) { return user->getName() == name; }
    );
    if (it != _users.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

User* ClusterManager::getTrackedUser() {
    auto it = std::find_if(
        _users.begin(),
        _users.end(),
        [](const std::unique_ptr<User>& u) { return u->isTracked(); }
    );
    if (it != _users.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

NetworkManager::NetworkMode ClusterManager::getNetworkMode() const {
    return _netMode;
}

void ClusterManager::setNetworkMode(NetworkManager::NetworkMode nm) {
    _netMode = nm;
}

void ClusterManager::setSceneTransform(glm::mat4 mat) {
    _sceneTransform = std::move(mat);
}

void ClusterManager::setSceneOffset(glm::vec3 offset) {
    _sceneTranslate = glm::translate(glm::mat4(1.f), std::move(offset));
    calculateSceneTransform();
}

void ClusterManager::setSceneRotation(float yaw, float pitch, float roll) {
    _sceneRotation = glm::yawPitchRoll(yaw, pitch, roll);
    calculateSceneTransform();
}

void ClusterManager::setSceneRotation(glm::mat4 mat) {
    _sceneRotation = std::move(mat);
    calculateSceneTransform();
}

bool ClusterManager::getIgnoreSync() const {
    return _ignoreSync;
}

void ClusterManager::setUseIgnoreSync(bool state) {
    _ignoreSync = state;
}

void ClusterManager::setUseASCIIForExternalControl(bool useASCII) {
    _useASCIIForExternalControl = useASCII;
}

bool ClusterManager::getUseASCIIForExternalControl() const {
    return _useASCIIForExternalControl;
}

void ClusterManager::setSceneScale(float scale) {
    _sceneScale = glm::scale(glm::mat4(1.f), glm::vec3(scale));
    calculateSceneTransform();
}

void ClusterManager::calculateSceneTransform() {
    _sceneTransform = _sceneRotation * _sceneTranslate * _sceneScale;
}

const std::string& ClusterManager::getMasterAddress() const {
    return _masterAddress;
}

void ClusterManager::setMasterAddress(std::string address) {
    std::transform(
        address.begin(),
        address.end(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _masterAddress = std::move(address);
}

int ClusterManager::getExternalControlPort() const {
    return _externalControlPort;
}

void ClusterManager::setExternalControlPort(int port) {
    _externalControlPort = port;
}

int ClusterManager::getNumberOfNodes() const {
    return static_cast<int>(_nodes.size());
}

const glm::mat4& ClusterManager::getSceneTransform() const {
    return _sceneTransform;
}

void ClusterManager::setThisNodeId(int id) {
    _thisNodeId = id;
}

int ClusterManager::getThisNodeId() const {
    return _thisNodeId;
}

bool ClusterManager::getFirmFrameLockSyncStatus() const {
    return _firmFrameLockSync;
}

void ClusterManager::setFirmFrameLockSyncStatus(bool state) {
    _firmFrameLockSync = state;
}

void ClusterManager::setMeshImplementation(MeshImplementation impl) {
    _meshImpl = impl;
}

ClusterManager::MeshImplementation ClusterManager::getMeshImplementation() const {
    return _meshImpl;
}

TrackingManager& ClusterManager::getTrackingManager() {
    return _trackingManager;
}

} // namespace sgct::core
