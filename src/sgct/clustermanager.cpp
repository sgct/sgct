/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/clustermanager.h>

#include <sgct/user.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct::core {

ClusterManager* ClusterManager::mInstance = nullptr;

ClusterManager* ClusterManager::instance()  {
    if (!mInstance) {
        mInstance = new ClusterManager();
    }

    return mInstance;
}

void ClusterManager::destroy() {
    delete mInstance;
    mInstance = nullptr;
}

ClusterManager::ClusterManager() {
    mUsers.push_back(std::make_unique<User>("default"));
}

void ClusterManager::addNode(std::unique_ptr<Node> node) {
    nodes.push_back(std::move(node));
}

void ClusterManager::addUser(std::unique_ptr<User> userPtr) {
    mUsers.push_back(std::move(userPtr));
}

Node* ClusterManager::getNode(size_t index) {
    return (index < nodes.size()) ? nodes[index].get() : nullptr;
}

Node* ClusterManager::getNode(const std::string& name) {
    auto it = std::find_if(
        nodes.begin(),
        nodes.end(),
        [&name](const std::unique_ptr<Node>& n) { return n->getName() == name; }
    );
    if (it != nodes.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

Node* ClusterManager::getThisNode() {
    return mThisNodeId < 0 ? nullptr : nodes[mThisNodeId].get();
}

User& ClusterManager::getDefaultUser() {
    // This object is guaranteed to exist as we add it in the constructor and it is not
    // possible to clear the mUsers list
    return *mUsers[0];
}

User* ClusterManager::getUser(const std::string& name) {
    auto it = std::find_if(
        mUsers.begin(),
        mUsers.end(),
        [&name](const std::unique_ptr<User>& user) { return user->getName() == name; }
    );
    if (it != mUsers.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

User* ClusterManager::getTrackedUser() {
    auto it = std::find_if(
        mUsers.begin(),
        mUsers.end(),
        [](const std::unique_ptr<User>& u) { return u->isTracked(); }
    );
    if (it != mUsers.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

NetworkManager::NetworkMode ClusterManager::getNetworkMode() const {
    return mNetMode;
}

void ClusterManager::setNetworkMode(NetworkManager::NetworkMode nm) {
    mNetMode = nm;
}

void ClusterManager::setSceneTransform(glm::mat4 mat) {
    mSceneTransform = std::move(mat);
}

void ClusterManager::setSceneOffset(glm::vec3 offset) {
    mSceneTranslate = glm::translate(glm::mat4(1.f), std::move(offset));
    calculateSceneTransform();
}

void ClusterManager::setSceneRotation(float yaw, float pitch, float roll) {
    mSceneRotation = glm::yawPitchRoll(yaw, pitch, roll);
    calculateSceneTransform();
}

void ClusterManager::setSceneRotation(glm::mat4 mat) {
    mSceneRotation = std::move(mat);
    calculateSceneTransform();
}

bool ClusterManager::getIgnoreSync() const {
    return mIgnoreSync;
}

void ClusterManager::setUseIgnoreSync(bool state) {
    mIgnoreSync = state;
}

void ClusterManager::setUseASCIIForExternalControl(bool useASCII) {
    mUseASCIIForExternalControl = useASCII;
}

bool ClusterManager::getUseASCIIForExternalControl() const {
    return mUseASCIIForExternalControl;
}

void ClusterManager::setSceneScale(float scale) {
    mSceneScale = glm::scale(glm::mat4(1.f), glm::vec3(scale));
    calculateSceneTransform();
}

void ClusterManager::calculateSceneTransform() {
    mSceneTransform = mSceneRotation * mSceneTranslate * mSceneScale;
}

const std::string& ClusterManager::getMasterAddress() const {
    return mMasterAddress;
}

void ClusterManager::setMasterAddress(std::string address) {
    std::transform(
        address.begin(),
        address.end(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    mMasterAddress = std::move(address);
}

int ClusterManager::getExternalControlPort() const {
    return mExternalControlPort;
}

void ClusterManager::setExternalControlPort(int port) {
    mExternalControlPort = port;
}

size_t ClusterManager::getNumberOfNodes() const {
    return nodes.size();
}

const glm::mat4& ClusterManager::getSceneTransform() const {
    return mSceneTransform;
}

void ClusterManager::setThisNodeId(int id) {
    mThisNodeId = id;
}

int ClusterManager::getThisNodeId() const {
    return mThisNodeId;
}

bool ClusterManager::getFirmFrameLockSyncStatus() const {
    return mFirmFrameLockSync;
}

void ClusterManager::setFirmFrameLockSyncStatus(bool state) {
    mFirmFrameLockSync = state;
}

void ClusterManager::setMeshImplementation(MeshImplementation impl) {
    mMeshImpl = impl;
}

ClusterManager::MeshImplementation ClusterManager::getMeshImplementation() const {
    return mMeshImpl;
}

TrackingManager& ClusterManager::getTrackingManager() {
    return mTrackingManager;
}

} // namespace sgct::core
