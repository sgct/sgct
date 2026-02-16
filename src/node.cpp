/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/node.h>

#include <sgct/config.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace sgct {

Node::Node(const config::Node& node, bool initializeWindows)
    : _address(node.address)
    , _syncPort(node.port)
    , _dataTransferPort(node.dataTransferPort.value_or(0))
    , _useSwapGroups(node.swapLock.value_or(false))
{
    ZoneScoped;

    if (initializeWindows) {
        for (const config::Window& window : node.windows) {
            if (window.scalable.has_value()) {
#ifdef SGCT_HAS_SCALABLE
                auto win = std::make_unique<Window>(
                    createScalableConfiguration(*window.scalable, window)
                );
                addWindow(std::move(win));
#else // ^^^^ SGCT_HAS_SCALABLE // !SGCT_HAS_SCALABLE vvvv
                Log::Error(
                    "Trying to load a ScalableMesh configuration but the program was "
                    "compiled without support for it"
                );
#endif // SGCT_HAS_SCALABLE
            }
            else {
                auto win = std::make_unique<Window>(window);
                addWindow(std::move(win));
            }
        }
    }
}

void Node::addWindow(std::unique_ptr<Window> window) {
    _windows.emplace_back(std::move(window));
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
