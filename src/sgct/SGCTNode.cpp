/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTNode.h>

#include <sgct/MessageHandler.h>
#include <algorithm>

namespace sgct_core {

/*!
    Add a window to the window vector. Note that a window must be opened to become visible.
*/
void SGCTNode::addWindow(sgct::SGCTWindow window) {
    mWindows.push_back(std::move(window));
}

/*!
    Set which window that will render the draw calls.
*/
void SGCTNode::setCurrentWindowIndex(size_t index) {
    mCurrentWindowIndex = index;
}

/*!
    Set to true if this node's windows should belong to a nvida swap group. Only valid before window opens.
*/
void SGCTNode::setUseSwapGroups(bool state) {
    mUseSwapGroups = state;
}

/*!
    Check if a key is pressed for all windows.
*/
bool SGCTNode::getKeyPressed(int key) {
    if (key == GLFW_KEY_UNKNOWN) {
        return false;
    }

    for (std::size_t i = 0; i < mWindows.size(); i++) {
        if (glfwGetKey(mWindows[i].getWindowHandle(), key)) {
            return true;
        }
    }
    return false;
}

/*!
    Get the number of windows in the window vector
*/
size_t SGCTNode::getNumberOfWindows() {
    return mWindows.size();
}

/*!
    Get the window pointer at index in window vector.
*/
sgct::SGCTWindow* SGCTNode::getWindowPtr(size_t index) {
    return &mWindows[index];
}

/*!
    Get the active window pointer.
*/
sgct::SGCTWindow* SGCTNode::getCurrentWindowPtr() {
    return &mWindows[mCurrentWindowIndex];
}

/*! Get the current window index */
size_t SGCTNode::getCurrentWindowIndex() {
    return mCurrentWindowIndex;
}

/*!
    Check if all windows are set to close and close them.
*/
bool SGCTNode::shouldAllWindowsClose() {
    size_t counter = 0;
    for (size_t i = 0; i < mWindows.size(); i++) {
        if (glfwWindowShouldClose(mWindows[i].getWindowHandle())) {
            mWindows[i].setVisibility(false);
            glfwSetWindowShouldClose(mWindows[i].getWindowHandle(), GL_FALSE);
        }
    }

    for (size_t i = 0; i < mWindows.size(); i++) {
        if (!(mWindows[i].isVisible() || mWindows[i].isRenderingWhileHidden())) {
            counter++;
        }
    }

    return counter == mWindows.size();
}

/*!
    Show all hidden windows.
*/
void SGCTNode::showAllWindows() {
    for (size_t i = 0; i < mWindows.size(); i++) {
        mWindows[i].setVisibility(true);
    }
}

/*!
    Is this node using nvidia swap groups for it's windows?
*/
bool SGCTNode::isUsingSwapGroups() {
    return mUseSwapGroups;
}

/*!
    Hide all windows.
*/
void SGCTNode::hideAllWindows() {
    for (size_t i = 0; i < mWindows.size(); i++) {
        mWindows[i].setVisibility(false);
    }
}

/*!
\param address is the hostname, DNS-name or ip
*/
void SGCTNode::setAddress(std::string address) {
    std::transform(address.begin(), address.end(), address.begin(), ::tolower);
    mAddress = std::move(address);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_DEBUG,
        "SGCTNode: Setting address to %s\n",
        mAddress.c_str()
    );
}

/*!
\param sync port is the number of the tcp port used for communication with this node
*/
void SGCTNode::setSyncPort(std::string port) {
    mSyncPort = std::move(port);
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_DEBUG,
        "SGCTNode: Setting sync port to %s\n",
        mSyncPort.c_str()
    );
}

/*!
\param data transfer port is the number of the tcp port used for data transfers to this node
*/
void SGCTNode::setDataTransferPort(std::string port) {
    mDataTransferPort = std::move(port);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_DEBUG,
        "SGCTNode: Setting data transfer port to %s\n",
        mDataTransferPort.c_str()
    );
}

/*!
\param name the name identification string of this node
*/
void SGCTNode::setName(std::string name) {
    mName = std::move(name);
}

/*!
\returns the address of this node
*/
std::string SGCTNode::getAddress() const {
    return mAddress;
}

/*!
\returns the sync port of this node
*/
std::string SGCTNode::getSyncPort() const {
    return mSyncPort;
}

/*!
\returns the data transfer port of this node
*/
std::string SGCTNode::getDataTransferPort() const {
    return mDataTransferPort;
}

/*!
\returns the name if this node
*/
std::string SGCTNode::getName() const {
    return mName;
}

} // namespace sgct_core
