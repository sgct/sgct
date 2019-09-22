/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/node.h>

#include <sgct/messagehandler.h>
#include <algorithm>

namespace sgct::core {

void Node::addWindow(Window window) {
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

    for (const Window& window : mWindows) {
        if (glfwGetKey(window.getWindowHandle(), key)) {
            return true;
        }
    }
    return false;
}

int Node::getNumberOfWindows() {
    return static_cast<int>(mWindows.size());
}

Window& Node::getWindow(int index) {
    return mWindows[index];
}

Window& Node::getCurrentWindow() {
    return mWindows[mCurrentWindowIndex];
}

int Node::getCurrentWindowIndex() {
    return mCurrentWindowIndex;
}

bool Node::shouldAllWindowsClose() {
    for (Window& window : mWindows) {
        if (glfwWindowShouldClose(window.getWindowHandle())) {
            window.setVisibility(false);
            glfwSetWindowShouldClose(window.getWindowHandle(), 0);
        }
    }

    size_t counter = 0;
    for (const Window& window : mWindows) {
        if (!(window.isVisible() || window.isRenderingWhileHidden())) {
            counter++;
        }
    }

    return counter == mWindows.size();
}

void Node::showAllWindows() {
    for (Window& window : mWindows) {
        window.setVisibility(true);
    }
}

void Node::hideAllWindows() {
    for (Window& window : mWindows) {
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

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Node: Setting address to %s\n", mAddress.c_str()
    );
}

void Node::setSyncPort(int port) {
    mSyncPort = port;
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Node: Setting sync port to %d\n", mSyncPort
    );
}

void Node::setDataTransferPort(int port) {
    mDataTransferPort = port;

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Node: Setting data transfer port to %d\n", mDataTransferPort
    );
}

void Node::setName(std::string name) {
    mName = std::move(name);
}

const std::string& Node::getAddress() const {
    return mAddress;
}

int Node::getSyncPort() const {
    return mSyncPort;
}

int Node::getDataTransferPort() const {
    return mDataTransferPort;
}

const std::string& Node::getName() const {
    return mName;
}

} // namespace sgct::core
