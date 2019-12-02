/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/node.h>

#include <sgct/config.h>
#include <sgct/keys.h>
#include <sgct/logger.h>
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
    Logger::Debug("Setting address to %s", address.c_str());

    _syncPort = node.port;
    Logger::Debug("Setting sync port to %d", _syncPort);

    if (node.dataTransferPort) {
        _dataTransferPort = *node.dataTransferPort;
        Logger::Debug("Setting data transfer port to %d", _dataTransferPort);
    }
    if (node.swapLock) {
        _useSwapGroups = *node.swapLock;
    }

    for (const config::Window& window : node.windows) {
        std::unique_ptr<Window> win = std::make_unique<Window>(getNumberOfWindows());
        win->applyWindow(window);
        addWindow(std::move(win));
    }
}

void Node::addWindow(std::unique_ptr<Window> window) {
    _windows.emplace_back(std::move(window));
}

bool Node::getKeyPressed(Key key) {
    if (key == Key::Unknown) {
        return false;
    }

    for (const std::unique_ptr<Window>& window : _windows) {
        if (glfwGetKey(window->getWindowHandle(), static_cast<int>(key))) {
            return true;
        }
    }
    return false;
}

int Node::getNumberOfWindows() const {
    return static_cast<int>(_windows.size());
}

Window& Node::getWindow(int index) {
    return *_windows[index];
}

const Window& Node::getWindow(int index) const {
    return *_windows[index];
}

bool Node::closeAllWindows() {
    for (std::unique_ptr<Window>& window : _windows) {
        if (glfwWindowShouldClose(window->getWindowHandle())) {
            window->setVisible(false);
            glfwSetWindowShouldClose(window->getWindowHandle(), 0);
        }
    }

    size_t counter = 0;
    for (const std::unique_ptr<Window>& window : _windows) {
        if (!(window->isVisible() || window->isRenderingWhileHidden())) {
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
