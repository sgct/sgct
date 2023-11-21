/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__MPCDI__H__
#define __SGCT__MPCDI__H__

#include <sgct/sgctexports.h>
#include <sgct/config.h>
#include <sgct/math.h>
#include <string>
#include <vector>

namespace sgct::mpcdi {

struct SGCT_EXPORT ReturnValue {
    /// The window resolution
    ivec2 resolution = ivec2{ 0, 0 };
    struct ViewportInfo {
        /// The configuration struct for the individual viewports
        config::MpcdiProjection proj;
        /// The warping mesh for each viewport as included inthe MPCDI file
        std::vector<char> meshData;
    };
    /// The list of all viewports in the MPCDI
    std::vector<ViewportInfo> viewports;
};

/**
 * \throw std::runtime_error if the parsing fails
 */
SGCT_EXPORT ReturnValue parseMpcdiConfiguration(const std::string& filename);

} //namespace sgct::mpcdi

#endif // __SGCT__MPCDI__H__
