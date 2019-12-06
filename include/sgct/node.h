/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__NODE__H__
#define __SGCT__NODE__H__

#include <sgct/keys.h>
#include <sgct/window.h>
#include <string>
#include <vector>

namespace sgct::config { struct Node; }

namespace sgct {

class Node {
public:
    void applyNode(const config::Node& node);

    /// Add a window to this node. Note that a window must be opened to become
    void addWindow(std::unique_ptr<Window> window);

    /// Check if all windows are set to close and close them.
    bool closeAllWindows();

    /// Is this node using nvidia swap groups for it's windows?
    bool isUsingSwapGroups() const;

    /// Check if a key is pressed for all windows.
    bool isKeyPressed(Key key);

    const std::vector<std::unique_ptr<Window>>& windows() const;

    /// \return the address of this node
    const std::string& address() const;

    /// \return the sync port of this node
    int syncPort() const;

    /// \return the data transfer port of this node
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
