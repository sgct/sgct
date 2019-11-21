/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__NODE__H__
#define __SGCT__NODE__H__

#include <sgct/window.h>
#include <string>
#include <vector>

namespace sgct::config { struct Node; }

namespace sgct::core {

class Node {
public:
    void applyNode(const config::Node& node);

    /**
     * Add a window to the window vector. Note that a window must be opened to become
     * visible.
     */
    void addWindow(Window window);

    /// Check if all windows are set to close and close them.
    bool closeAllWindows();

    /// Is this node using nvidia swap groups for it's windows?
    bool isUsingSwapGroups() const;

    /// Check if a key is pressed for all windows.
    bool getKeyPressed(int key);

    /// Get the number of windows in the window vector
    int getNumberOfWindows() const;

    /// Get the window pointer at index in window vector.
    Window& getWindow(int index);

    /// Get the window pointer at index in window vector.
    const Window& getWindow(int index) const;

    /// \return the address of this node
    const std::string& getAddress() const;

    /// \return the sync port of this node
    int getSyncPort() const;

    /// \return the data transfer port of this node
    int getDataTransferPort() const;

private:
    std::string _address;
    int _syncPort = 0;
    int _dataTransferPort = 0;

    std::vector<Window> _windows;
    bool _useSwapGroups = false;
};

} // namespace sgct::core

#endif // __SGCT__NODE__H__
