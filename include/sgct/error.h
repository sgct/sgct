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
#include <string>

namespace sgct {

/*
    1000s: Config
    1000: User / Tracking device name must not be empty
    1001: User / Tracking tracker name must not be empty
    1010: Capture / Mono path must not be empty
    1011: Capture / Left path must not be empty
    1012: Capture / Right path must not be empty
    1020: Settings / Swap interval must not be negative
    1021: Settings / Refresh rate must not be negative
    1022: Settings / OSDText font name must not be negative
    1023: Settings / OSDText font path must not be empty
    1024: Settings / OSDText font size must not be negative
    1025: Settings / FXAA trim must be postive
    1030: Device / Device name must not be empty
    1031: Device / VRPN address for sensors must not be empty
    1032: Device / VRPN address for buttons must not be empty
    1033: Device / VRPN address for axes must not be empty
    1040: Tracker / Tracker name must not be empty
    1050: Planar Projection / Up and down field of views can not be the same
    1051: Planar Projection / Left and right field of views can not be the same
    1060: Fisheye Projection / Field of view setting must be positive
    1061: Fisheye Projection / Left and right crop must not overlap
    1062: Fisheye Projection / Bottom and top crop must not overlap
    1063: Fisheye Projection / Quality value must be positive
    1064: Fisheye Projection / Quality setting only allows powers of two
    1065: Fisheye Projection / Diameter must be positive
    1066: Fisheye Projection / Every background color component has to be positive
    1070: Spherical Mirror Projection / Quality value must be positive
    1071: Spherical Mirror Projection / Quality setting only allows powers of two
    1072: Spherical Mirror Projection / Every background color component has to be positive
    1080: Spout Output Projection / Mapping name must not be empty
    1081: Spout Output Projection / Quality value must be positive
    1082: Spout Output Projection / Quality setting only allows powers of two
    1083: Spout Output Projection / Every background color component has to be positive
    1090: Viewport / User must not be empty
    1091: Viewport / Overlay texture path must not be empty
    1092: Viewport / Blendmask texture path must not be empty
    1093: Viewport / Blendmask level texture path must not be empty
    1094: Viewport / Correction mesh texture path must not be empty
    1095: Viewport / Mesh hint must not be empty
    1100: Window / Window name must not be empty
    1101: Window / Empty tags are not allowed for windows
    1102: Window / Gamma value must be at least 0.1
    1103: Window / Contrast value must be postive
    1104: Window / Brightness value must be positive
    1105: Window / Number of MSAA samples must be non-negative
    1106: Window / Monitor index must be non-negative
    1107: Window / MPCDI file must not be empty
    1110: Node / Node address must not be empty
    1111: Node / Node port must be non-negative
    1112: Node / Node data transfer port must be non-negative
    1120: Cluster / Cluster master address must not be empty
    1121: Cluster / Cluster external control port must be non-negative
    1122: Cluster / More than one unnamed users specified in the cluster

    2000s: CorrectionMeshes
    2000: CorrectionMesh / "Failed to export. Geometry type is not supported"
    2001: CorrectionMesh / "Failed to export " + exportPath + ". Failed to open"
    2002: DomeProjection / "Failed to open " + path
    2010: MPCDIMesh / Error reading from file. Could not find lines
    2011: MPCDIMesh / Invalid header information in MPCDI mesh
    2012: MPCDIMesh / Incorrect file type. Unknown header type
    2020: OBJ / Failed to open warping mesh file
    2021: OBJ / Vertex count doesn't match number of texture coordinates
    2030: PaulBourke / Failed to open warping mesh file
    2031: PaulBourke / Error reading mapping type
    2032: PaulBourke / Invalid data
    2040: Scalable / Failed to open warping mesh file
    2041: Scalable / Incorrect mesh data geometry
    2050: SCISS / Failed to open warping mesh file
    2051: SCISS / Incorrect file id
    2052: SCISS / Error parsing file version from file
    2053: SCISS / Error parsing type from file
    2054: SCISS / Error parsing view data from file
    2055: SCISS / Error parsing file
    2056: SCISS / Error parsing vertices from file
    2057: SCISS / Error parsing indices from file
    2058: SCISS / Error parsing faces from file
    2060: SimCAD / Error parsing XML file
    2061: SimCAD / Error reading XML file. Missing 'GeometryFile'
    2062: SimCAD / Error reading XML file. Missing 'GeometryDefinition'
    2063: SimCAD / Not the same x coords as y coords
    2064: SimCAD / Not a valid squared matrix read from SimCAD file
    2070: SkySkan / Not a valid squared matrix read from SimCAD file
    2071: SkySkan / Data reading error

    3000s: Engine
    3000: Engine / Failed to initialize GLFW
    3001: Engine / Computer is not a part of the cluster configuration
    3002: Engine / Requested node id was not found in the cluster configuration
    3003: Engine / Error initializing network connections
    3004: Engine / No windows exist in configuration
    3005: Engine / No windows created on this node
    3006: Engine / No sync signal from master after X seconds
    3007: Engine / No sync signal from slaves after X seconds
    3010: Engine / GLFW error

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
    6010: ProjectionPlane / Failed parsing coordinates. Missing XML children
    6011: ProjectionPlane / Failed parsing ProjectionPlane coordinates. Type error
    6020: Viewport / Failed to parse position. Type error
    6021: Viewport / Failed to parse size. Type error
    6030: Window / Could not parse window size. Type error
    6040: Node / Missing field address in node
    6041: Node / Missing field port in node
    6050: Settings / Wrong buffer precision value. Must be 16 or 32
    6051: Settings / Wrong buffer precision value type
    6060: Capture / Unknown capturing format. Needs to be png, tga, jpg
    6070: Tracker / Tracker is missing 'name'
    6080: XML Parsing / No XML file provided
    6081: XML Parsing / Error parsing XML file
    6082: XML Parsing / Cannot find 'Cluster' node
    6083: XML Parsing / Cannot find master address or DNS name in XML
    6084: XML Parsing / SGCT doesn't support usage of % in the path
    6085: XML Parsing / Bad configuration path string
    6086: XML Parsing / Cannot fetch environment variable

    7000s: Shader Handling
    7000: ShaderManager / Cannot add shader program %s: Already exists
    7001: ShaderManager / Could not find shader with name %s
    7010: ShaderProgram / No shaders have been added to the program %s
    7011: ShaderProgram / Error creating the program %s
    7012: ShaderProgram / Error linking the program %s

    8000s: Window
    8000: Error resolving swapgroup functions
    8001: Error opening window

    9000s: Image
    9000: Image / Cannot load empty filepath
    9001: Image / Could not open file '%s' for loading image
    9002: Image / Filename not set for saving image
    9003: Image / Cannot save file %s
    9004: Image / Could not save file %s as PNG
    9005: Image / Could not save file %s as JPG
    9006: Image / Could not save file %s as TGA
*/

struct Error : public std::runtime_error {
    enum class Component {
        Config,
        CorrectionMesh,
        DomeProjection,
        Engine,
        Image,
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

    Error(Component comp, int c, std::string msg);

    Component component;
    int code;
    std::string message;
};

} // namespace sgct

#endif // __SGCT__ERROR__H__
