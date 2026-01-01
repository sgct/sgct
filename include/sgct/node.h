/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NODE__H__
#define __SGCT__NODE__H__

#include <sgct/sgctexports.h>

#include <sgct/window.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sgct::config { struct Node; }

namespace sgct {

class SGCT_EXPORT Node {
public:
    Node(const config::Node& node, bool initializeWindows);

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
    const std::string _address;
    const uint16_t _syncPort;
    const uint16_t _dataTransferPort;
    const bool _useSwapGroups;

    std::vector<std::unique_ptr<Window>> _windows;
};

} // namespace sgct

#endif // __SGCT__NODE__H__
