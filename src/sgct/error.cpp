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
            default: throw std::logic_error("Unhandled component");
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

/*
    1000s: Config
    1000: User / Tracking device name must not be empty
    1001: User / Tracking tracker name must not be empty
    1002: Capture / Mono path must not be empty
    1003: Capture / Left path must not be empty
    1004: Capture / Right path must not be empty
    1005: Settings / Swap interval must not be negative
    1006: Settings / Refresh rate must not be negative
    1007: Settings / OSDText font name must not be negative
    1008: Settings / OSDText font path must not be empty
    1009: Settings / OSDText font size must not be negative
    1010: Settings / FXAA trim must be postive
    1011: Device / Device name must not be empty
    1012: Device / VRPN address for sensors must not be empty
    1013: Device / VRPN address for buttons must not be empty
    1014: Device / VRPN address for axes must not be empty
    1015: Tracker / Tracker name must not be empty
    1016: Planar Projection / Up and down field of views can not be the same
    1017: Planar Projection / Left and right field of views can not be the same
    1018: Fisheye Projection / Field of view setting must be positive
    1019: Fisheye Projection / Left and right crop must not overlap
    1020: Fisheye Projection / Bottom and top crop must not overlap
    1021: Fisheye Projection / Quality value must be positive
    1022: Fisheye Projection / Quality setting only allows powers of two
    1023: Fisheye Projection / Diameter must be positive
    1024: Fisheye Projection / Every background color component has to be positive
    1025: Spherical Mirror Projection / Quality value must be positive
    1026: Spherical Mirror Projection / Quality setting only allows powers of two
    1027: Spherical Mirror Projection / Every background color component has to be positive
    1028: Spout Output Projection / Mapping name must not be empty
    1029: Spout Output Projection / Quality value must be positive
    1030: Spout Output Projection / Quality setting only allows powers of two
    1031: Spout Output Projection / Every background color component has to be positive
    1032: Viewport / User must not be empty
    1033: Viewport / Overlay texture path must not be empty
    1034: Viewport / Blendmask texture path must not be empty
    1035: Viewport / Blendmask level texture path must not be empty
    1036: Viewport / Correction mesh texture path must not be empty
    1037: Viewport / Mesh hint must not be empty
    1038: Window / Window name must not be empty
    1039: Window / Empty tags are not allowed for windows
    1040: Window / Gamma value must be at least 0.1
    1041: Window / Contrast value must be postive
    1042: Window / Brightness value must be positive
    1043: Window / Number of MSAA samples must be non-negative
    1044: Window / Monitor index must be non-negative
    1045: Window / MPCDI file must not be empty
    1046: Node / Node address must not be empty
    1047: Node / Node port must be non-negative
    1048: Node / Node name must not be empty
    1049: Node / Node data transfer port must be non-negative
    1050: Cluster / Cluster master address must not be empty
    1051: Cluster / Cluster external control port must be non-negative

    2000s: CorrectionMeshes
    2000: CorrectionMesh / "Failed to export. Geometry type is not supported"
    2001: CorrectionMesh / "Failed to export " + exportPath + ". Failed to open"
    2002: CorrectionMesh / "Error loading mesh, not path was specified"
    2003: DomeProjection / "Failed to open " + path
    2004: MPCDIMesh / Error reading from file. Could not find lines
    2005: MPCDIMesh / Invalid header information in MPCDI mesh
    2006: MPCDIMesh / Incorrect file type. Unknown header type
    2007: OBJ / Failed to open warping mesh file
    2008: OBJ / Vertex count doesn't match number of texture coordinates
    2009: PaulBourke / Failed to open warping mesh file
    2010: PaulBourke / Invalid data
    2011: Scalable / Failed to open warping mesh file
    2012: Scalable / Incorrect mesh data geometry
    2013: SCISS / Failed to open warping mesh file
    2014: SCISS / Incorrect file id
    2015: SCISS / Error parsing file version from file
    2016: SCISS / Error parsing type from file
    2017: SCISS / Error parsing view data from file
    2018: SCISS / Error parsing vertices from file
    2019: SCISS / Error parsing indices from file
    2020: SCISS / Error parsing faces from file
    2021: SimCAD / Error parsing XML file
    2022: SimCAD / Error reading XML file. Missing 'GeometryFile'
    2023: SimCAD / Error reading XML file. Missing 'GeometryDefinition'
    2024: SimCAD / Not the same x coords as y coords
    2025: SimCAD / Not a valid squared matrix read from SimCAD file
    2026: SkySkan / Not a valid squared matrix read from SimCAD file
    2027: SkySkan / Data reading error

    3000s: Engine
    3000: Engine / Failed to initialize GLFW
    3001: Engine / Computer is not a part of the cluster configuration
    3002: Engine / Error initializing network connections
    3003: Engine / No windows exist in configuration
    3004: Engine / Failed to open window
    3005: Engine / No windows created on this node
    3006: Engine / No sync signal from master after X seconds
    3007: Engine / No sync signal from slaves after X seconds

    4000s: MPCDI
    4000: MPCDI / Failed to parse position from XML
    4001: MPCDI / Failed to parse size from XML
    4002: MPCDI / Missing child element 'frustum'
    4003: MPCDI / Failed to parse frustum element. Missing element
    4004: MPCDI / Failed to parse frustum element. Conversion error
    4005: MPCDI / Require both xResolution and yResolution values
    4006: MPCDI / No 'id' attribute provided for region
    4007: MPCDI / Multiple 'buffer' elements not supported
    4008: MPCDI / GeometryWarpFile requires interpolation
    4009: MPCDI / Only linear interpolation is supported
    4010: MPCDI / GeometryWarpFile requires path
    4011: MPCDI / No matching geometryWarpFile found
    4012: MPCDI / Cannot find XML root
    4013: MPCDI / Error parsing MPCDI, missing or wrong 'profile'
    4014: MPCDI / Error parsing MPCDI, missing or wrong 'geometry'
    4015: MPCDI / Error parsing MPCDI, missing or wrong 'version'
    4016: MPCDI / Missing 'display' element
    4017: MPCDI / Multiple 'display' elements not supported
    4018: MPCDI / Missing 'files' element
    4019: MPCDI / Unable to open zip archive file
    4020: MPCDI / Unable to get zip archive info
    4021: MPCDI / Unable to get info on file
    4022: MPCDI / Unable to open XML file
    4023: MPCDI / Read from XML file failed
    4024: MPCDI / Unable to open PFM file
    4025: MPCDI / Read from PFM file failed
    4026: MPCDI / MPCDI does not contain the XML and/or PFM file
    4027: MPCDI / Error parsing main XML file
    
    5000s: Network
    5000: Network / Failed to parse hints for connection
    5001: Network / Failed to listen init socket
    5002: Network / Bind socket failed
    5003: Network / Listen failed
    5004: Network / Failed to init client socket
    5005: Network / Winsock 2.2 startup failed
    5006: Network / Failed to get host name

    6000s: XML configuration parsing
    6000: PlanarProjection / Missing specification of field-of-view values
    6001: PlanarProjection / Failed to parse planar projection FOV
    6002: ProjectionPlane / Failed parsing coordinates. Missing XML children
    6003: ProjectionPlane / Failed parsing ProjectionPlane coordinates. Type error
    6004: Viewport / Failed to parse position. Type error
    6005: Viewport / Failed to parse size. Type error
    6006: Window / Could not parse window size. Type error
    6007: Node / Missing field address in node
    6008: Settings / Wrong buffer precision value. Must be 16 or 32
    6009: Settings / Wrong buffer precision value type
    6010: Capture / Unknown capturing format. Needs to be png, tga, jpg
    6011: Tracker / Tracker is missing 'name'
    6012: XML Parsing / No XML file provided
    6013: XML Parsing / Error parsing XML file
    6014: XML Parsing / Cannot find 'Cluster' node
    6015: XML Parsing / Cannot find master address or DNS name in XML
    6016: XML Parsing / SGCT doesn't support usage of % in the path
    6017: XML Parsing / Bad configuration path string
    6018: XML Parsing / Cannot fetch environment variable

    7000s: Shader Handling
    7000: ShaderManager / Cannot add shader program %s: Already exists
    7001: ShaderManager / Could not find shader with name %s
    7002: ShaderProgram / No shaders have been added to the program %s
    7003: ShaderProgram / Error creating the program %s
    7004: ShaderProgram / Error linking the program %s

    8000s: Window
    8000: Error resolving swapgroup functions
*/