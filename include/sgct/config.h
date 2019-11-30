/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CONFIG__H__
#define __SGCT__CONFIG__H__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace sgct::config {

struct User {
    struct Tracking {
        std::string tracker;
        std::string device;
    };

    std::optional<std::string> name;
    std::optional<float> eyeSeparation;
    std::optional<glm::vec3> position;
    std::optional<glm::mat4> transformation;
    std::optional<Tracking> tracking;
};
void validateUser(const User& user);

struct Capture {
    enum class Format { PNG, JPG, TGA };
    std::optional<std::string> monoPath;
    std::optional<std::string> leftPath;
    std::optional<std::string> rightPath;
    std::optional<Format> format;
};
void validateCapture(const Capture& capture);

struct Scene {
    std::optional<glm::vec3> offset;
    std::optional<glm::quat> orientation;
    std::optional<float> scale;
};
void validateScene(const Scene& scene);

struct Settings {
    enum class BufferFloatPrecision { Float16Bit, Float32Bit};

    struct Display {
        std::optional<int> swapInterval;
        std::optional<int> refreshRate;
        std::optional<bool> keepAspectRatio;
        std::optional<bool> exportWarpingMeshes;
    };

    struct OSDText {
        std::optional<std::string> name;
        std::optional<std::string> path;
        std::optional<int> size;
        std::optional<float> xOffset;
        std::optional<float> yOffset;
    };

    struct FXAA {
        std::optional<float> offset;
        std::optional<float> trim;
    };

    std::optional<bool> useDepthTexture;
    std::optional<bool> useNormalTexture;
    std::optional<bool> usePositionTexture;
    std::optional<BufferFloatPrecision> bufferFloatPrecision;
    std::optional<Display> display;
    std::optional<OSDText> osdText;
    std::optional<FXAA> fxaa;
};
void validateSettings(const Settings& settings);

struct Device {
    struct Sensors {
        std::string vrpnAddress;
        int identifier = -1;
    };
    struct Buttons {
        std::string vrpnAddress;
        int count = 0;
    };
    struct Axes {
        std::string vrpnAddress;
        int count = 0;
    };
    
    std::string name;
    std::vector<Sensors> sensors;
    std::vector<Buttons> buttons;
    std::vector<Axes> axes;
    std::optional<glm::vec3> offset;
    std::optional<glm::mat4> transformation;
};
void validateDevice(const Device& device);

struct Tracker {
    std::string name;
    std::vector<Device> devices;
    std::optional<glm::vec3> offset;
    std::optional<double> scale;
    std::optional<glm::mat4> transformation;
};
void validateTracker(const Tracker& tracker);

struct NoProjection {};
struct PlanarProjection {
    struct FOV {
        float down = 0.f;
        float left = 0.f;
        float right = 0.f;
        float up = 0.f;
        std::optional<float> distance;
    };
    FOV fov;
    std::optional<glm::quat> orientation;
    std::optional<glm::vec3> offset;
};
void validatePlanarProjection(const PlanarProjection& proj);

struct FisheyeProjection {
    enum class Method { FourFace, FiveFace };
    enum class Interpolation { Linear, Cubic };
    struct Crop {
        // Crop values are measured as [0,1] from the respective side, which is why right
        // and top are 0 and not 1 as one might expect
        float left = 0.f;
        float right = 0.f;
        float bottom = 0.f;
        float top = 0.f;
    };
    std::optional<float> fov;
    std::optional<int> quality;
    std::optional<Method> method;
    std::optional<Interpolation> interpolation;
    std::optional<float> tilt;
    std::optional<float> diameter;
    std::optional<Crop> crop;
    std::optional<glm::vec3> offset;
    std::optional<glm::vec4> background;
};
void validateFisheyeProjection(const FisheyeProjection& proj);

struct SphericalMirrorProjection {
    struct Mesh {
        std::string bottom;
        std::string left;
        std::string right;
        std::string top;
    };
    std::optional<int> quality;
    std::optional<float> tilt;
    std::optional<glm::vec4> background;
    Mesh mesh;
};
void validateSphericalMirrorProjection(const SphericalMirrorProjection& proj);

struct SpoutOutputProjection {
    enum class Mapping { Fisheye, Equirectangular, Cubemap };
    struct Channels {
        bool right = true;
        bool zLeft = true;
        bool bottom = true;
        bool top = true;
        bool left = true;
        bool zRight = true;
    };
    std::optional<int> quality;
    std::optional<Mapping> mapping;
    std::string mappingSpoutName;
    std::optional<glm::vec4> background;
    std::optional<Channels> channels;
    std::optional<glm::vec3> orientation;
};
void validateSpoutOutputProjection(const SpoutOutputProjection& proj);

struct ProjectionPlane {
    glm::vec3 lowerLeft = glm::vec3(0.f);
    glm::vec3 upperLeft = glm::vec3(0.f);
    glm::vec3 upperRight = glm::vec3(0.f);
};
void validateProjectionPlane(const ProjectionPlane& proj);

struct MpcdiProjection {
    struct Frustum {
        float down;
        float up;
        float left;
        float right;
    };
    std::optional<std::string> id;
    std::optional<glm::vec2> position;
    std::optional<glm::vec2> size;
    std::optional<glm::vec2> resolution;
    std::optional<Frustum> frustum;
    std::optional<float> distance;
    std::optional<glm::quat> orientation;
    std::optional<glm::vec3> offset;
};
void validateMpcdiProjection(const MpcdiProjection& proj);

struct Viewport {
    enum class Eye { Mono, StereoLeft, StereoRight };
    
    std::optional<std::string> user;
    std::optional<std::string> name;
    std::optional<std::string> overlayTexture;
    std::optional<std::string> blendMaskTexture;
    std::optional<std::string> blendLevelMaskTexture;
    std::optional<std::string> correctionMeshTexture;
    std::optional<std::string> meshHint;
    std::optional<bool> isTracked;
    std::optional<Eye> eye;
    std::optional<glm::vec2> position;
    std::optional<glm::vec2> size;
    
    std::variant<NoProjection, PlanarProjection, FisheyeProjection,
        SphericalMirrorProjection, SpoutOutputProjection, ProjectionPlane> projection;
};
void validateViewport(const Viewport& viewport);

struct Window {
    enum class ColorBitDepth {
        Depth8,
        Depth16,
        Depth16Float,
        Depth32Float,
        Depth16Int,
        Depth32Int,
        Depth16UInt,
        Depth32UInt
    };

    enum class StereoMode {
        NoStereo = 0,
        Active,
        AnaglyphRedCyan,
        AnaglyphAmberBlue,
        AnaglyphRedCyanWimmer,
        Checkerboard,
        CheckerboardInverted,
        VerticalInterlaced,
        VerticalInterlacedInverted,
        Dummy,
        SideBySide,
        SideBySideInverted,
        TopBottom,
        TopBottomInverted
    };

    std::optional<std::string> name;
    std::vector<std::string> tags;
    std::optional<ColorBitDepth> bufferBitDepth;
    std::optional<bool> isFullScreen;
    std::optional<bool> isFloating;
    std::optional<bool> alwaysRender;
    std::optional<bool> isHidden;
    std::optional<bool> doubleBuffered;
    std::optional<float> gamma;
    std::optional<float> contrast;
    std::optional<float> brightness;
    std::optional<int> msaa;
    std::optional<bool> hasAlpha;
    std::optional<bool> useFxaa;
    std::optional<bool> isDecorated;
    std::optional<bool> hasBorder;
    std::optional<bool> draw2D;
    std::optional<bool> draw3D;
    std::optional<bool> blitPreviousWindow;
    std::optional<int> monitor;
    std::optional<std::string> mpcdi;
    std::optional<StereoMode> stereo;
    std::optional<glm::ivec2> pos;
    glm::ivec2 size = glm::ivec2(1);
    std::optional<glm::ivec2> resolution;

    std::vector<Viewport> viewports;
};
void validateWindow(const Window& window);

struct Node {
    std::string address;
    int port = 0;
    std::optional<int> dataTransferPort;
    std::optional<bool> swapLock;
    std::vector<Window> windows;
};
void validateNode(const Node& node);

struct Cluster {
    std::string masterAddress;
    std::optional<bool> debugLog;
    std::optional<bool> checkOpenGL;
    std::optional<bool> checkFBOs;
    std::optional<int> externalControlPort;
    std::optional<bool> firmSync;
    std::optional<Scene> scene;
    std::vector<Node> nodes;
    std::vector<User> users;
    std::optional<Capture> capture;
    std::vector<Tracker> trackers;
    std::optional<Settings> settings;
};
void validateCluster(const Cluster& cluster);

} // namespace sgct::config

#endif // __SGCT__CONFIG__H__
