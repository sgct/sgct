/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/node.h>

#include <sgct/keys.h>
#include <sgct/messagehandler.h>
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace sgct::core {

void Node::addWindow(Window window) {
    _windows.emplace_back(std::move(window));
}

// @TODO (abock, 2019-08-29): I think this state of 'current window index' can probably go
// away. It seems like an extra state machine that is not worth carrying around for the
// few use cases that it has
void Node::setCurrentWindowIndex(int index) {
    _currentWindowIndex = index;
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

int Node::getNumberOfWindows() {
    return static_cast<int>(_windows.size());
}

Window& Node::getWindow(int index) {
    return _windows[index];
}

Window& Node::getCurrentWindow() {
    return _windows[_currentWindowIndex];
}

int Node::getCurrentWindowIndex() const {
    return _currentWindowIndex;
}

bool Node::shouldAllWindowsClose() {
    for (Window& window : _windows) {
        if (glfwWindowShouldClose(window.getWindowHandle())) {
            window.setVisibility(false);
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
        window.setVisibility(true);
    }
}

void Node::hideAllWindows() {
    for (Window& window : _windows) {
        window.setVisibility(false);
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

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Node: Setting address to %s\n", _address.c_str()
    );
}

void Node::setSyncPort(int port) {
    _syncPort = port;
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Node: Setting sync port to %d\n", _syncPort
    );
}

void Node::setDataTransferPort(int port) {
    _dataTransferPort = port;

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Node: Setting data transfer port to %d\n", _dataTransferPort
    );
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
