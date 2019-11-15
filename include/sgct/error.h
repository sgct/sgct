/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__ERROR__H__
#define __SGCT__ERROR__H__

#include <stdexcept>

namespace sgct {

struct Error : public std::runtime_error {
    enum class Component {
        Config,
        CorrectionMesh,
        DomeProjection,
        Engine,
        MPCDI,
        MPCDIMesh,
        Network,
        OBJ,
        PaulBourke,
        ReadConfig,
        Scalable,
        SCISS,
        Shader,
        SimCAD,
        SkySkan,
        Window
    };

    Error(Component component, int code, std::string message);

    Component component;
    int code;
    std::string message;
};

} // namespace sgct

#endif // __SGCT__ERROR__H__
