/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__ERROR__H__
#define __SGCT__ERROR__H__

#include <sgct/sgctexports.h>
#include <stdexcept>
#include <string>

namespace sgct {

/*
 * 1000s: Configuration
 * 1000: User / Eye separation must be zero or a positive number
 * 1001: User / Tracking device name must not be empty
 * 1002: User / Tracking tracker name must not be empty
 * 1003: User / Name 'default' is not permitted for a user
 * 1010: Capture / Capture path must not be empty
 * 1011: Capture / Screenshot ranges beginning has to be before the end
 * 1020: Settings / Swap interval must not be negative
 * 1021: Settings / Refresh rate must not be negative
 * 1030: Device / Device name must not be empty
 * 1031: Device / VRPN address for sensors must not be empty
 * 1032: Device / VRPN address for buttons must not be empty
 * 1033: Device / VRPN address for axes must not be empty
 * 1040: Tracker / Tracker name must not be empty
 * 1050: Planar Projection / Up and down field of views can not be the same
 * 1051: Planar Projection / Left and right field of views can not be the same
 * 1060: Fisheye Projection / Field of view setting must be positive
 * 1061: Fisheye Projection / Left and right crop must not overlap
 * 1062: Fisheye Projection / Bottom and top crop must not overlap
 * 1063: Fisheye Projection / Quality value must be positive
 * 1064: Fisheye Projection / Quality setting only allows powers of two
 * 1065: Fisheye Projection / Diameter must be positive
 * 1066: Fisheye Projection / All background color components have to be positive
 * 1070: Spherical Mirror Projection / Quality value must be positive
 * 1071: Spherical Mirror Projection / Quality setting only allows powers of two
 * 1072: Spherical Mirror Projection / All background color components have to be positive
 * 1080: Spout Output Projection / Mapping name must not be empty
 * 1081: Spout Output Projection / Quality value must be positive
 * 1082: Spout Output Projection / Quality setting only allows powers of two
 * 1083: Spout Output Projection / All background color components have to be positive
 * 1090: Viewport / User must not be empty
 * 1091: Viewport / Overlay texture path must not be empty
 * 1092: Viewport / Blendmask texture path must not be empty
 * 1093: Viewport / Blendmask level texture path must not be empty
 * 1094: Viewport / Correction mesh texture path must not be empty
 * 1095: Viewport / No valid projection provided
 * 1100: Window / Window name must not be empty
 * 1101: Window / Empty tags are not allowed for windows
 * 1102: Window / Number of MSAA samples must be non-negative
 * 1103: Window / Monitor index must be non-negative or -1
 * 1105: Window / Window must contain at least one viewport
 * 1107: Window / Window id must be non-negative and unique [%i used multiple times]
 * 1108: Window / Tried to configure window %i to be blitted from window %i, but no such
                  window was specified
 * 1109: Window / Window %i tried to blit from itself, which cannot work
 * 1110: Node / Node address must not be empty
 * 1111: Node / Node port must be non-negative
 * 1112: Node / Node data transfer port must be non-negative
 * 1113: Node / Every node must contain at least one window
 * 1120: Cluster / Cluster master address must not be empty
 * 1122: Cluster / There must be at least one user in the cluster
 * 1123: Cluster / More than one unnamed users specified in the cluster
 * 1124: Cluster / No two users can have the same name
 * 1125: Cluster / All trackers specified in the 'User's have to be valid tracker names
 * 1127: Cluster / Configuration must contain at least one node
 * 1128: Cluster / Two or more nodes are using the same port

 * 2000s: Correction Meshes
 * 2000: CorrectionMesh / Failed to export. Geometry type is not supported"
 * 2001: CorrectionMesh / Failed to export " + exportPath + ". Failed to open"
 * 2010: DomeProjection / Failed to open '%s'
 * 2030: OBJ / Failed to open '%s'
 * 2031: OBJ / Vertex count doesn't match number of texture coordinates in '%s'
 * 2032: OBJ / Faces in mesh '%s' referenced vertices that were undefined
 * 2033: OBJ / Faces in mesh '%s' are using relative index positions that are unsupported
 * 2034: OBJ / Illegal vertex format in OBJ file '%s' in line '%s'
 * 2035: OBJ / Illegal face format in OBJ file '%s' in line '%s'
 * 2036: OBJ / Illegal vertex format in OBJ file '%s' in line '%s'
 * 2040: PaulBourke / Failed to open file '%s'
 * 2041: PaulBourke / Error reading mapping type in file '%s'
 * 2042: PaulBourke / Invalid data in file '%s'
 * 2050: Pfm / Failed to open '%s'
 * 2051: Pfm / Error reading from file '%s'
 * 2052: Pfm / Invalid header syntax in file '%s'
 * 2053: Pfm / Incorrect file type in file '%s'
 * 2054: Pfm / Error reading correction values in file '%s'
 * 2060: Scalable / Failed to open file '%s'
 * 2061: Scalable / Incorrect mesh data geometry in file '%s'
 * 2062: Scalable / Illegal formatting of face in file '%s' in line '%s'
 * 2070: SCISS / Failed to open '%s'
 * 2071: SCISS / Incorrect file id in file '%s'
 * 2072: SCISS / Error parsing file version from file '%s'
 * 2073: SCISS / Error parsing type from file '%s'
 * 2074: SCISS / Error parsing view data from file '%s"
 * 2075: SCISS / Error parsing file '%s'
 * 2076: SCISS / Error parsing vertices from file '%s'
 * 2077: SCISS / Error parsing indices from file '%s'
 * 2078: SCISS / Error parsing faces from file '%s'
 * 2080: SimCAD / Error parsing file '%s'. %s
 * 2081: SimCAD / Error reading file '{}'. Missing 'GeometryFile'
 * 2082: SimCAD / Error reading file '{}'. Missing 'GeometryDefinition'
 * 2083: SimCAD / Not the same x coords as y coords
 * 2084: SimCAD / Not a valid squared matrix read from SimCAD file
 * 2090: SkySkan / Failed to open file '%s'
 * 2091: SkySkan / Data reading error in file '%s'

 * 3000s: Engine
 * 3000: Engine / Failed to initialize GLFW
 * 3001: Engine / Requested node id was not found in the cluster configuration
 * 3002: Engine / When running locally, a node ID needs to be specified
 * 3003: Engine / Computer is not a part of the cluster configuration
 * 3004: Engine / No sync signal from master after X seconds
 * 3005: Engine / No sync signal from clients after X seconds
 * 3006: Engine / Error requesting maximum number of swap groups
 * 3010: Engine / GLFW error

 * 5000s: Network
 * 5000: Network / Failed to parse hints for connection
 * 5001: Network / Failed to listen init socket
 * 5002: Network / Bind socket failed
 * 5003: Network / Listen failed
 * 5004: Network / Failed to init client socket
 * 5005: Network / Failed to set network no-delay option: %s
 * 5006: Network / Failed to set reuse address: %s
 * 5007: Network / Failed to set send buffer size to %d. %s
 * 5008: Network / Failed to set receive buffer size to %d. %s
 * 5009: Network / Failed to set keep alive: %s
 * 5010: Network / Error in sync frame %i for connection %i
 * 5011: Network / Failed to uncompress data for connection %i: %s // Sync Connection
 * 5012: Network / Failed to uncompress data for connection %i: %s // Data Transfer
 * 5013: Network / TCP connection %i receive failed: %s
 * 5014: Network / Send data failed: %s
 * 5020: NetworkManager / Winsock 2.2 startup failed
 * 5021: NetworkManager / No address information for this node available
 * 5022: NetworkManager / No address information for master available
 * 5023: NetworkManager / Port %i is already used by connection %i
 * 5024: NetworkManager / Failed to compress data: %s
 * 5025: NetworkManager / No port provided for connection to %s
 * 5026: NetworkManager / Empty address for connection to %i
 * 5027: NetworkManager / Failed to get host name
 * 5028: NetworkManager / Failed to get address info: %s

 * 6000s: Configuration parsing
 * 6000: PlanarProjection / Missing specification of field-of-view values
 * 6001: PlanarProjection / Failed to parse planar projection FOV
 * 6010: ProjectionPlane / Failed parsing coordinates. Missing children
 * 6011: ProjectionPlane / Failed parsing ProjectionPlane coordinates. Type error
 * 6020: Viewport / Unrecognized eye position
 * 6021: Viewport / Failed to parse position. Type error
 * 6022: Viewport / Failed to parse size. Type error
 * 6023: Viewport / Unrecognized interpolation
 * 6030: Window / Could not parse window position. Type error
 * 6031: Window / Could not parse window size. Type error
 * 6032: Window / Could not parse window resolution. Type error
 * 6040: Node / Missing field address in node
 * 6041: Node / Missing field port in node
 * 6050: Settings / Wrong buffer precision value. Must be 16 or 32
 * 6051: Settings / Wrong buffer precision value type
 * 6060: Capture / Unknown capturing format. Needs to be png, tga, jpg
 * 6070: Tracker / Tracker is missing 'name'
 * 6080: Parsing / No file provided
 * 6081: Parsing / Could not find configureation file: %s
 * 6082: Parsing / Error parsing file
 * 6083: Parsing / Cannot find 'Cluster' node
 * 6084: Parsing / Cannot find master address
 * 6085: Parsing / Unknown stereo mode %s
 * 6086: Parsing / Unknown color bit depth %s
 * 6087: Parsing / Unknown resolution %s for cube map
 * 6088: Parsing / Unsupported file extension %s
 * 6089: Parsing / Validation against schema failed
 * 6090: SpoutOutput / Unknown spout output mapping: %s
 * 6100: SphericalMirror / Missing geometry paths
 * 6110: TextureMappedProjection / Missing correction mesh

 * 7000s: Shader Handling
 * 7000: ShaderManager / Cannot add shader program %s: Already exists
 * 7001: ShaderManager / Could not find shader with name %s
 * 7010: ShaderProgram / No shaders have been added to the program %s
 * 7011: ShaderProgram / Error linking the program %s
 * 7012: ShaderProgram / Failed to create shader program %s: Already created
 * 7013: ShaderProgram / Failed to create shader program %s: Unknown error

 * 8000s: Window
 * 8000: Window / Error opening GLFW window

 * 9000s: Image
 * 9000: Image / Cannot load empty filepath
 * 9001: Image / Could not open file '%s' for loading image
 * 9002: Image / Filename not set for saving image
 * 9003: Image / Cannot save file %s
 * 9004: Image / Could not save file %s as JPG
 * 9005: Image / Could not save file %s as TGA
 * 9006: Image / Missing image data to save PNG
 * 9007: Image / Can't save %d bit
 * 9008: Image / Can't create PNG file '%s'
 * 9009: Image / Failed to create PNG struct
 * 9010: Image / Failed to create PNG info struct
 * 9011: Image / One of the called PNG functions failed
 * 9012: Image / Invalid image size %i x %i %i channels

 OBS:  When adding a new error code, don't forget to update docs/errors.md accordingly
 */

struct SGCT_EXPORT Error : public std::runtime_error {
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

    const Component component;
    const int code;
    const std::string message;
};

} // namespace sgct

#endif // __SGCT__ERROR__H__
