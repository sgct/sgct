/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__NODE__H__
#define __SGCT__NODE__H__

#include <sgct/window.h>
#include <string>
#include <vector>

namespace sgct_core {

class Node {
public:
    /**
     * Add a window to the window vector. Note that a window must be opened to become
     * visible.
     */
    void addWindow(sgct::Window window);

    /// Set which window that will render the draw calls.
    void setCurrentWindowIndex(int index);

    /**
     * Set to true if this node's windows should belong to a nvida swap group. Only valid
     * before window opens.
     */
    void setUseSwapGroups(bool state);

    /// Check if all windows are set to close and close them.
    bool shouldAllWindowsClose();

    /// Is this node using nvidia swap groups for it's windows?
    bool isUsingSwapGroups() const;

    /// Show all hidden windows.
    void showAllWindows();

    /// Hide all windows.
    void hideAllWindows();

    /// Check if a key is pressed for all windows.
    bool getKeyPressed(int key);

    /// Get the number of windows in the window vector
    int getNumberOfWindows();

    /// Get the window pointer at index in window vector.
    sgct::Window& getWindow(int index);

    /// Get the active window pointer.
    sgct::Window& getCurrentWindow();

    /// Get the current window index
    int getCurrentWindowIndex();

    /// \param address is the hostname, DNS-name or ip
    void setAddress(std::string address);

    /**
     * \param sync port is the number of the tcp port used for communication with this
     * node
     */
    void setSyncPort(std::string port);

    /**
     * \param data transfer port is the number of the tcp port used for data transfers to 
     * this node
     */
    void setDataTransferPort(std::string port);

    /// \param name the name identification string of this node
    void setName(std::string name);

    /// \returns the address of this node
    const std::string& getAddress() const;

    /// \returns the sync port of this node
    const std::string& getSyncPort() const;

    /// \returns the data transfer port of this node
    const std::string& getDataTransferPort() const;

    /// \returns the name if this node
    const std::string& getName() const;

private:
    std::string mName;
    std::string mAddress;
    std::string mSyncPort;
    std::string mDataTransferPort;

    int mCurrentWindowIndex = 0;
    std::vector<sgct::Window> mWindows;
    bool mUseSwapGroups = false;
};

} // namespace sgct_core

#endif // __SGCT__NODE__H__
