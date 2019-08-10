/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__NODE__H__
#define __SGCT__NODE__H__

#include <sgct/SGCTWindow.h>
#include <string>
#include <vector>

namespace sgct_core {

class SGCTNode {
public:
    bool getKeyPressed(int key);

    /*!
        Get the number of windows in the window vector
    */
    size_t getNumberOfWindows();

    /*!
        Get the window pointer at index in window vector.
    */
    sgct::SGCTWindow& getWindowPtr(size_t index);
    
    /*!
        Get the active window pointer.
    */
    sgct::SGCTWindow& getCurrentWindowPtr();

    /*! Get the current window index */
    size_t getCurrentWindowIndex();

    void addWindow(sgct::SGCTWindow window);
    void setCurrentWindowIndex(size_t index);
    void setUseSwapGroups(bool state);

    bool shouldAllWindowsClose();
    bool isUsingSwapGroups();
    void showAllWindows();
    void hideAllWindows();

    void setAddress(std::string address);
    void setSyncPort(std::string port);
    void setDataTransferPort(std::string port);
    void setName(std::string name);
    std::string getAddress() const;
    std::string getSyncPort() const;
    std::string getDataTransferPort() const;
    std::string getName() const;

private:
    std::string mName;
    std::string mAddress;
    std::string mSyncPort;
    std::string mDataTransferPort;

    size_t mCurrentWindowIndex = 0;
    std::vector<sgct::SGCTWindow> mWindows;
    bool mUseSwapGroups = false;
};

} // namespace sgct_core

#endif // __SGCT__NODE__H__
