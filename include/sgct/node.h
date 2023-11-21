/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NODE__H__
#define __SGCT__NODE__H__

#include <sgct/sgctexports.h>
#include <sgct/keys.h>
#include <sgct/window.h>
#include <string>
#include <vector>

namespace sgct::config { struct Node; }

namespace sgct {

class SGCT_EXPORT Node {
public:
    Node() = default;
    Node(const Node&) = delete;
    Node(Node&&) = default;
    Node& operator=(const Node&) = delete;
    Node& operator=(Node&&) = default;


    void applyNode(const config::Node& node, bool initializeWindows);

    /**
     * Add a window to this node.
     */
    void addWindow(std::unique_ptr<Window> window);

    /**
     * Check if all windows are set to close and close them.
     */
    bool closeAllWindows();

    /**
     * Is this node using nvidia swap groups for its windows?
     */
    bool isUsingSwapGroups() const;

    /**
     * Check if a key is pressed for all windows.
     */
    bool isKeyPressed(Key key);

    const std::vector<std::unique_ptr<Window>>& windows() const;

    /**
     * \return the address of this node
     */
    const std::string& address() const;

    /**
     * \return the sync port of this node
     */
    int syncPort() const;

    /**
     * \return the data transfer port of this node
     */
    int dataTransferPort() const;

private:
    std::string _address;
    int _syncPort = 0;
    int _dataTransferPort = 0;

    std::vector<std::unique_ptr<Window>> _windows;
    bool _useSwapGroups = false;
};

} // namespace sgct

#endif // __SGCT__NODE__H__
