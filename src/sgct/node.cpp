/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/node.h>

#include <sgct/messagehandler.h>
#include <algorithm>

namespace sgct_core {

void Node::addWindow(sgct::Window window) {
    mWindows.emplace_back(std::move(window));
}

// @TODO (abock, 2019-08-29): I think this state of 'current window index' can probably go
// away. It seems like an extra state machine that is not worth carrying around for the
// few use cases that it has
void Node::setCurrentWindowIndex(int index) {
    mCurrentWindowIndex = index;
}

void Node::setUseSwapGroups(bool state) {
    mUseSwapGroups = state;
}

bool Node::getKeyPressed(int key) {
    if (key == GLFW_KEY_UNKNOWN) {
        return false;
    }

    for (const sgct::Window& window : mWindows) {
        if (glfwGetKey(window.getWindowHandle(), key)) {
            return true;
        }
    }
    return false;
}

int Node::getNumberOfWindows() {
    return static_cast<int>(mWindows.size());
}

sgct::Window& Node::getWindow(int index) {
    return mWindows[index];
}

sgct::Window& Node::getCurrentWindow() {
    return mWindows[mCurrentWindowIndex];
}

int Node::getCurrentWindowIndex() {
    return mCurrentWindowIndex;
}

bool Node::shouldAllWindowsClose() {
    for (sgct::Window& window : mWindows) {
        if (glfwWindowShouldClose(window.getWindowHandle())) {
            window.setVisibility(false);
            glfwSetWindowShouldClose(window.getWindowHandle(), 0);
        }
    }

    size_t counter = 0;
    for (const sgct::Window& window : mWindows) {
        if (!(window.isVisible() || window.isRenderingWhileHidden())) {
            counter++;
        }
    }

    return counter == mWindows.size();
}

void Node::showAllWindows() {
    for (sgct::Window& window : mWindows) {
        window.setVisibility(true);
    }
}

void Node::hideAllWindows() {
    for (sgct::Window& window : mWindows) {
        window.setVisibility(false);
    }
}

bool Node::isUsingSwapGroups() const {
    return mUseSwapGroups;
}

void Node::setAddress(std::string address) {
    std::transform(
        address.begin(),
        address.end(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    mAddress = std::move(address);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Node: Setting address to %s\n", mAddress.c_str()
    );
}

void Node::setSyncPort(std::string port) {
    mSyncPort = std::move(port);
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Node: Setting sync port to %s\n", mSyncPort.c_str()
    );
}

void Node::setDataTransferPort(std::string port) {
    mDataTransferPort = std::move(port);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Node: Setting data transfer port to %s\n", mDataTransferPort.c_str()
    );
}

void Node::setName(std::string name) {
    mName = std::move(name);
}

const std::string& Node::getAddress() const {
    return mAddress;
}

const std::string& Node::getSyncPort() const {
    return mSyncPort;
}

const std::string& Node::getDataTransferPort() const {
    return mDataTransferPort;
}

const std::string& Node::getName() const {
    return mName;
}

} // namespace sgct_core
