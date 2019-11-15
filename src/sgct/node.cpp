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
    setAddress(node.address);
    if (node.name) {
        setName(*node.name);
    }
    setSyncPort(node.port);
    if (node.dataTransferPort) {
        setDataTransferPort(*node.dataTransferPort);
    }
    if (node.swapLock) {
        setUseSwapGroups(*node.swapLock);
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

void Node::setUseSwapGroups(bool state) {
    _useSwapGroups = state;
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

void Node::showAllWindows() {
    for (Window& window : _windows) {
        window.setVisible(true);
    }
}

void Node::hideAllWindows() {
    for (Window& window : _windows) {
        window.setVisible(false);
    }
}

bool Node::isUsingSwapGroups() const {
    return _useSwapGroups;
}

void Node::setAddress(std::string address) {
    std::transform(
        address.cbegin(),
        address.cend(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _address = std::move(address);

    MessageHandler::printDebug("Setting address to %s", _address.c_str());
}

void Node::setSyncPort(int port) {
    _syncPort = port;
    MessageHandler::printDebug("Setting sync port to %d", _syncPort);
}

void Node::setDataTransferPort(int port) {
    _dataTransferPort = port;

    MessageHandler::printDebug("Setting data transfer port to %d", _dataTransferPort);
}

void Node::setName(std::string name) {
    _name = std::move(name);
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

const std::string& Node::getName() const {
    return _name;
}

} // namespace sgct::core
