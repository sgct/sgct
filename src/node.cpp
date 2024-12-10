/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/node.h>

#include <sgct/config.h>
#include <sgct/keys.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace sgct {

void Node::applyNode(const config::Node& node, bool initializeWindows) {
    ZoneScoped;

    // Set network address
    std::string address = node.address;
    std::transform(
        address.cbegin(),
        address.cend(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _address = address;

    _syncPort = node.port;

    if (node.dataTransferPort) {
        _dataTransferPort = *node.dataTransferPort;
    }
    if (node.swapLock) {
        _useSwapGroups = *node.swapLock;
    }

    if (initializeWindows) {
        for (const config::Window& window : node.windows) {
            auto win = std::make_unique<Window>();
            win->applyWindow(window);
            addWindow(std::move(win));
        }
    }
}

void Node::addWindow(std::unique_ptr<Window> window) {
    _windows.emplace_back(std::move(window));
}

bool Node::isKeyPressed(Key key) {
    if (key == Key::Unknown) {
        return false;
    }

    for (const std::unique_ptr<Window>& window : _windows) {
        if (glfwGetKey(window->windowHandle(), static_cast<int>(key))) {
            return true;
        }
    }
    return false;
}

const std::vector<std::unique_ptr<Window>>& Node::windows() const {
    return _windows;
}

bool Node::closeAllWindows() {
    for (std::unique_ptr<Window>& window : _windows) {
        if (glfwWindowShouldClose(window->windowHandle())) {
            window->setVisible(false);
            glfwSetWindowShouldClose(window->windowHandle(), 0);
        }
    }

    const size_t counter = std::count_if(
        _windows.cbegin(),
        _windows.cend(),
        [](const std::unique_ptr<Window>& window) {
            return !(window->isVisible() || window->isRenderingWhileHidden());
        }
    );
    return (counter == _windows.size());
}

bool Node::isUsingSwapGroups() const {
    return _useSwapGroups;
}

const std::string& Node::address() const {
    return _address;
}

int Node::syncPort() const {
    return _syncPort;
}

int Node::dataTransferPort() const {
    return _dataTransferPort;
}

} // namespace sgct
