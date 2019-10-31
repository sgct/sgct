/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MPCDI__H__
#define __SGCT__MPCDI__H__

#include <sgct/config.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace sgct::core::mpcdi {

struct ReturnValue {
    /// The window resolution
    glm::ivec2 resolution;
    struct ViewportInfo {
        /// The configuration struct for the individual viewports
        config::MpcdiProjection proj;
        /// The warping mesh for each viewport as included inthe MPCDI file
        std::vector<char> meshData;
    };
    /// The list of all viewports in the MPCDI
    std::vector<ViewportInfo> viewports;
};

// throws std::runtime_error if the parsing fails
ReturnValue parseMpcdiConfiguration(const std::string& filename);

} //namespace sgct::core::mpcdi

#endif // __SGCT__MPCDI__H__
