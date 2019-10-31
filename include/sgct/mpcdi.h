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
    glm::ivec2 position;
    glm::ivec2 resolution;
    struct ViewportInfo {
        config::MpcdiProjection proj;
        std::vector<char> meshData;
    };
    std::vector<ViewportInfo> viewports;
};

// throws std::runtime_error if the parsing fails
ReturnValue parseConfiguration(const std::string& filenameMpcdi);

} //namespace sgct::core::mpcdi

#endif // __SGCT__MPCDI__H__
