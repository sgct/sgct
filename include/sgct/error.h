/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__ERROR__H__
#define __SGCT__ERROR__H__

#include <sgct/sgctexports.h>
#include <stdexcept>
#include <string>

namespace sgct {

struct SGCT_EXPORT Error final : public std::runtime_error {
    enum class Component {
        Config,
        CorrectionMesh,
        DomeProjection,
        Engine,
        Image,
        Network,
        OBJ,
        PaulBourke,
        Pfm,
        ReadConfig,
        Scalable,
        SCISS,
        Shader,
        SimCAD,
        SkySkan,
        Window
    };

    Error(Component comp, int c, std::string msg);
    ~Error() noexcept override;

    const Component component;
    const int code;
    const std::string message;
};

} // namespace sgct

#endif // __SGCT__ERROR__H__
