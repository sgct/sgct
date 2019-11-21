/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/node.h>

#include <sgct/keys.h>
#include <sgct/messagehandler.h>
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace sgct::core {

void Node::applyNode(const config::Node& node) {
    // Set network address
    std::string address = node.address;
    std::transform(
        address.cbegin(),
        address.cend(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _address = address;
    MessageHandler::printDebug("Setting address to %s", address.c_str());

    _syncPort = node.port;
    MessageHandler::printDebug("Setting sync port to %d", _syncPort);

    if (node.dataTransferPort) {
        _dataTransferPort = *node.dataTransferPort;
        MessageHandler::printDebug("Setting data transfer port to %d", _dataTransferPort);
    }
    if (node.swapLock) {
        _useSwapGroups = *node.swapLock;
    }

    for (const config::Window& window : node.windows) {
        Window win = Window(getNumberOfWindows());
        win.applyWindow(window);
        addWindow(std::move(win));
    }
}

void Node::addWindow(Window window) {
    _windows.emplace_back(std::move(window));
}

bool Node::getKeyPressed(int key) {
    if (key == key::Unknown) {
        return false;
    }

    for (const Window& window : _windows) {
        if (glfwGetKey(window.getWindowHandle(), key)) {
            return true;
        }
    }
    return false;
}

int Node::getNumberOfWindows() const {
    return static_cast<int>(_windows.size());
}

Window& Node::getWindow(int index) {
    return _windows[index];
}

const Window& Node::getWindow(int index) const {
    return _windows[index];
}

bool Node::closeAllWindows() {
    for (Window& window : _windows) {
        if (glfwWindowShouldClose(window.getWindowHandle())) {
            window.setVisible(false);
            glfwSetWindowShouldClose(window.getWindowHandle(), 0);
        }
    }

    size_t counter = 0;
    for (const Window& window : _windows) {
        if (!(window.isVisible() || window.isRenderingWhileHidden())) {
            counter++;
        }
    }

    return (counter == _windows.size());
}

bool Node::isUsingSwapGroups() const {
    return _useSwapGroups;
}

const std::string& Node::getAddress() const {
    return _address;
}

int Node::getSyncPort() const {
    return _syncPort;
}

int Node::getDataTransferPort() const {
    return _dataTransferPort;
}

} // namespace sgct::core
