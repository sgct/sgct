/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/node.h>

#include <sgct/messagehandler.h>
#include <algorithm>

namespace sgct_core {

void SGCTNode::addWindow(sgct::SGCTWindow window) {
    mWindows.emplace_back(std::move(window));
}

// @TODO (abock, 2019-08-29): I think this state of 'current window index' can probably go
// away. It seems like an extra state machine that is not worth carrying around for the
// few use cases that it has
void SGCTNode::setCurrentWindowIndex(int index) {
    mCurrentWindowIndex = index;
}

void SGCTNode::setUseSwapGroups(bool state) {
    mUseSwapGroups = state;
}

bool SGCTNode::getKeyPressed(int key) {
    if (key == GLFW_KEY_UNKNOWN) {
        return false;
    }

    for (const sgct::SGCTWindow& window : mWindows) {
        if (glfwGetKey(window.getWindowHandle(), key)) {
            return true;
        }
    }
    return false;
}

int SGCTNode::getNumberOfWindows() {
    return static_cast<int>(mWindows.size());
}

sgct::SGCTWindow& SGCTNode::getWindow(int index) {
    return mWindows[index];
}

sgct::SGCTWindow& SGCTNode::getCurrentWindow() {
    return mWindows[mCurrentWindowIndex];
}

int SGCTNode::getCurrentWindowIndex() {
    return mCurrentWindowIndex;
}

bool SGCTNode::shouldAllWindowsClose() {
    for (sgct::SGCTWindow& window : mWindows) {
        if (glfwWindowShouldClose(window.getWindowHandle())) {
            window.setVisibility(false);
            glfwSetWindowShouldClose(window.getWindowHandle(), 0);
        }
    }

    size_t counter = 0;
    for (const sgct::SGCTWindow& window : mWindows) {
        if (!(window.isVisible() || window.isRenderingWhileHidden())) {
            counter++;
        }
    }

    return counter == mWindows.size();
}

void SGCTNode::showAllWindows() {
    for (sgct::SGCTWindow& window : mWindows) {
        window.setVisibility(true);
    }
}

void SGCTNode::hideAllWindows() {
    for (sgct::SGCTWindow& window : mWindows) {
        window.setVisibility(false);
    }
}

bool SGCTNode::isUsingSwapGroups() const {
    return mUseSwapGroups;
}

void SGCTNode::setAddress(std::string address) {
    std::transform(
        address.begin(),
        address.end(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    mAddress = std::move(address);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTNode: Setting address to %s\n", mAddress.c_str()
    );
}

void SGCTNode::setSyncPort(std::string port) {
    mSyncPort = std::move(port);
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTNode: Setting sync port to %s\n", mSyncPort.c_str()
    );
}

void SGCTNode::setDataTransferPort(std::string port) {
    mDataTransferPort = std::move(port);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTNode: Setting data transfer port to %s\n", mDataTransferPort.c_str()
    );
}

void SGCTNode::setName(std::string name) {
    mName = std::move(name);
}

const std::string& SGCTNode::getAddress() const {
    return mAddress;
}

const std::string& SGCTNode::getSyncPort() const {
    return mSyncPort;
}

const std::string& SGCTNode::getDataTransferPort() const {
    return mDataTransferPort;
}

const std::string& SGCTNode::getName() const {
    return mName;
}

} // namespace sgct_core
