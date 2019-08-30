/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/ClusterManager.h>

#include <sgct/SGCTUser.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct_core {

ClusterManager* ClusterManager::mInstance = nullptr;

ClusterManager* ClusterManager::instance()  {
    if (!mInstance) {
        mInstance = new ClusterManager();
    }

    return mInstance;
}

void ClusterManager::destroy() {
    if (mInstance) {
        delete mInstance;
        mInstance = nullptr;
    }
}

ClusterManager::ClusterManager() {
    mUsers.push_back(std::make_unique<SGCTUser>("default"));
}

/*!
    Add a cluster node to the manager's vector.
*/
void ClusterManager::addNode(SGCTNode node) {
    nodes.push_back(std::move(node));
}

/*!
    Add an user ptr. The cluster manager will deallocate the user upon destruction.
*/
void ClusterManager::addUser(std::unique_ptr<SGCTUser> userPtr) {
    mUsers.push_back(std::move(userPtr));
}

/*!
    Get a pointer to a specific node.

    \param index the index to a node in the vector
    \returns the pointer to the requested node or NULL if not found
*/
SGCTNode* ClusterManager::getNodePtr(size_t index) {
    return (index < nodes.size()) ? &nodes[index] : nullptr;
}

/*!
    Get a pointer to a specific node.

    \param name of the node to search for
    \returns the pointer to the requested node or NULL if not found
*/
SGCTNode* ClusterManager::getNodePtr(const std::string& name) {
    auto it = std::find_if(
        nodes.begin(),
        nodes.end(),
        [&name](const SGCTNode& n) { return n.getName() == name; }
    );
    if (it != nodes.end()) {
        return &(*it);
    }
    else {
        return nullptr;
    }
}

/*!
    \returns a pointer to the node that this application is running on
*/
SGCTNode* ClusterManager::getThisNode() {
    return mThisNodeId < 0 ? nullptr : &nodes[mThisNodeId];
}

/*!
\returns the pointer to the default user
*/
SGCTUser* ClusterManager::getDefaultUser() {
    return mUsers[0].get();
}

/*!
\returns the pointer to a named user. NULL is returned if no user is found.
*/
SGCTUser* ClusterManager::getUserPtr(const std::string& name) {
    auto it = std::find_if(
        mUsers.begin(),
        mUsers.end(),
        [&name](const std::unique_ptr<SGCTUser>& user) { return user->getName() == name; }
    );
    if (it != mUsers.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

/*!
\returns the pointer to the tracked user. NULL is returned if no user is tracked.
*/
SGCTUser* ClusterManager::getTrackedUserPtr() {
    auto it = std::find_if(
        mUsers.begin(),
        mUsers.end(),
        [](const std::unique_ptr<SGCTUser>& u) { return u->isTracked(); }
    );
    if (it != mUsers.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

/*!
\returns the current network mode
*/
NetworkManager::NetworkMode ClusterManager::getNetworkMode() const {
    return mNetMode;
}

/*!
Sets the current network mode
*/
void ClusterManager::setNetworkMode(NetworkManager::NetworkMode nm) {
    mNetMode = nm;
}

/*!
Set the scene transform.
*/
void ClusterManager::setSceneTransform(glm::mat4 mat) {
    mSceneTransform = std::move(mat);
}

/*!
    Set the scene offset/translation. This is set using the XML config file for easier transitions between different hardware setups.
*/
void ClusterManager::setSceneOffset(glm::vec3 offset) {
    mSceneTranslate = glm::translate(glm::mat4(1.f), std::move(offset));
    calculateSceneTransform();
}

/*!
    Set the scene rotation. This is set using the XML config file for easier transitions between different hardware setups.
*/
void ClusterManager::setSceneRotation(float yaw, float pitch, float roll) {
    mSceneRotation = glm::yawPitchRoll(yaw, pitch, roll);
    calculateSceneTransform();
}

/*!
Set the scene rotation. This is set using the XML config file for easier transitions between different hardware setups.
*/
void ClusterManager::setSceneRotation(glm::mat4 mat) {
    mSceneRotation = std::move(mat);
    calculateSceneTransform();
}

/*!
Get if software sync between nodes is disabled
*/
bool ClusterManager::getIgnoreSync() const {
    return mIgnoreSync;
}

/*!
Set if software sync between nodes should be ignored
*/
void ClusterManager::setUseIgnoreSync(bool state) {
    mIgnoreSync = state;
}

/*!
    Set if external control should use ASCII (Telnet) or raw binary parsing.
*/
void ClusterManager::setUseASCIIForExternalControl(bool useASCII) {
    mUseASCIIForExternalControl = useASCII;
}

/*!
    Get if external control is using ASCII (Telnet) or raw binary parsing.
*/
bool ClusterManager::getUseASCIIForExternalControl() const {
    return mUseASCIIForExternalControl;
}

/*!
    Set the scene scale. This is set using the XML config file for easier transitions between different hardware setups.
*/
void ClusterManager::setSceneScale(float scale) {
    mSceneScale = glm::scale(glm::mat4(1.f), glm::vec3(scale));
    calculateSceneTransform();
}

/*!
Updates the scene transform. Calculates the transform matrix using: SceneTransform = Rotation * Offset * Scale
*/
void ClusterManager::calculateSceneTransform() {
    mSceneTransform = mSceneRotation * mSceneTranslate * mSceneScale;
}

/*!
        \returns the dns, name or ip of the master in the cluster (depends on what's been set in the XML config)
*/
const std::string& ClusterManager::getMasterAddress() const {
    return mMasterAddress;
}

/*!
    \param the dns, ip or name of the master in the cluster
*/
void ClusterManager::setMasterAddress(std::string address) {
    std::transform(address.begin(), address.end(), address.begin(), ::tolower);
    mMasterAddress = std::move(address);
}

/*!
\returns the external control port number if it's set or specified in the XML configuration
*/
const std::string& ClusterManager::getExternalControlPort() const {
    return mExternalControlPort;
}

/*!
\param the external control port number
*/
void ClusterManager::setExternalControlPort(std::string port) {
    mExternalControlPort = std::move(port);
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

sgct::SGCTTrackingManager& ClusterManager::getTrackingManager() {
    return mTrackingManager;
}

} // namespace sgct_core
