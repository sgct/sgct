/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/error.h>

#include <string>

namespace {
    std::string nameForComponent(sgct::Error::Component component) {
        switch (component) {
            case sgct::Error::Component::Config: return "Config";
            case sgct::Error::Component::CorrectionMesh: return "CorrectionMesh";
            case sgct::Error::Component::DomeProjection: return "DomeProjection";
            case sgct::Error::Component::Engine: return "Engine";
            case sgct::Error::Component::MPCDI: return "MPCDI";
            case sgct::Error::Component::MPCDIMesh: return "MPCDIMesh";
            case sgct::Error::Component::Network: return "Network";
            case sgct::Error::Component::OBJ: return "OBJ";
            case sgct::Error::Component::PaulBourke: return "PaulBourke";
            case sgct::Error::Component::ReadConfig: return "ReadConfig";
            case sgct::Error::Component::Scalable: return "Scalable";
            case sgct::Error::Component::SCISS: return "SCISS";
            case sgct::Error::Component::Shader: return "Shader";
            case sgct::Error::Component::SimCAD: return "SimCAD";
            case sgct::Error::Component::SkySkan: return "SkySkan";
            case sgct::Error::Component::Window: return "Window";
            default: throw std::logic_error("Unhandled case label");
        }
    }
} // namespace

namespace sgct {

Error::Error(Component component, int code, std::string message)
    : std::runtime_error(
        "[" + nameForComponent(component) + "] (" + std::to_string(code) + "): " + message
      )
    , component(component)
    , code(code)
    , message(std::move(message))
{}

} // namespace sgct
