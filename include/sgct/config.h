/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CONFIG__H__
#define __SGCT__CONFIG__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace sgct::config {

struct SGCT_EXPORT User {
    struct Tracking {
        std::string tracker;
        std::string device;
    };

    std::optional<std::string> name;
    std::optional<float> eyeSeparation;
    std::optional<vec3> position;
    std::optional<mat4> transformation;
    std::optional<Tracking> tracking;
};
SGCT_EXPORT void validateUser(const User& user);



struct SGCT_EXPORT Capture {
    enum class Format { PNG, JPG, TGA };
    struct ScreenShotRange {
        int first = -1; // inclusive
        int last = -1;  // exclusive
    };

    std::optional<std::string> path;
    std::optional<Format> format;
    std::optional<ScreenShotRange> range;
};
SGCT_EXPORT void validateCapture(const Capture& capture);



struct SGCT_EXPORT Scene {
    std::optional<vec3> offset;
    std::optional<quat> orientation;
    std::optional<float> scale;
};
SGCT_EXPORT void validateScene(const Scene& scene);



struct SGCT_EXPORT Settings {
    enum class BufferFloatPrecision { Float16Bit, Float32Bit};

    struct Display {
        std::optional<int> swapInterval;
        std::optional<int> refreshRate;
    };

    std::optional<bool> useDepthTexture;
    std::optional<bool> useNormalTexture;
    std::optional<bool> usePositionTexture;
    std::optional<BufferFloatPrecision> bufferFloatPrecision;
    std::optional<Display> display;
};
SGCT_EXPORT void validateSettings(const Settings& settings);



struct SGCT_EXPORT Device {
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
    std::optional<vec3> offset;
    std::optional<mat4> transformation;
};
SGCT_EXPORT void validateDevice(const Device& device);



struct SGCT_EXPORT Tracker {
    std::string name;
    std::vector<Device> devices;
    std::optional<vec3> offset;
    std::optional<double> scale;
    std::optional<mat4> transformation;
};
SGCT_EXPORT void validateTracker(const Tracker& tracker);



struct SGCT_EXPORT NoProjection {};



struct SGCT_EXPORT PlanarProjection {
    struct FOV {
        float down = 0.f;
        float left = 0.f;
        float right = 0.f;
        float up = 0.f;
        std::optional<float> distance;
    };
    FOV fov;
    std::optional<quat> orientation;
    std::optional<vec3> offset;
};
SGCT_EXPORT void validatePlanarProjection(const PlanarProjection& proj);



struct SGCT_EXPORT TextureMappedProjection : PlanarProjection { };
SGCT_EXPORT void validateTextureProjection(const TextureMappedProjection& proj);



struct SGCT_EXPORT FisheyeProjection {
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
    std::optional<Interpolation> interpolation;
    std::optional<float> tilt;
    std::optional<float> diameter;
    std::optional<Crop> crop;
    std::optional<bool> keepAspectRatio;
    std::optional<vec3> offset;
    std::optional<vec4> background;
};
SGCT_EXPORT void validateFisheyeProjection(const FisheyeProjection& proj);



struct SGCT_EXPORT SphericalMirrorProjection {
    struct Mesh {
        std::string bottom;
        std::string left;
        std::string right;
        std::string top;
    };
    std::optional<int> quality;
    std::optional<float> tilt;
    std::optional<vec4> background;
    Mesh mesh;
};
SGCT_EXPORT void validateSphericalMirrorProjection(const SphericalMirrorProjection& proj);



struct SGCT_EXPORT SpoutOutputProjection {
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
    std::optional<vec4> background;
    std::optional<Channels> channels;
    std::optional<vec3> orientation;
    std::optional<bool> drawMain;
};
SGCT_EXPORT void validateSpoutOutputProjection(const SpoutOutputProjection& proj);



struct SGCT_EXPORT SpoutFlatProjection {
    PlanarProjection proj;
    std::optional<int> width;
    std::optional<int> height;
    std::string mappingSpoutName;
    std::optional<vec4> background;
    std::optional<bool> drawMain;
};
SGCT_EXPORT void validateSpoutFlatProjection(const SpoutFlatProjection& proj);



struct SGCT_EXPORT CylindricalProjection {
    std::optional<int> quality;
    std::optional<float> rotation;
    std::optional<float> heightOffset;
    std::optional<float> radius;
};
SGCT_EXPORT void validateCylindricalProjection(const CylindricalProjection& proj);



struct SGCT_EXPORT EquirectangularProjection {
    std::optional<int> quality;
};
SGCT_EXPORT void validateEquirectangularProjection(const EquirectangularProjection& proj);


struct SGCT_EXPORT ProjectionPlane {
    vec3 lowerLeft = vec3{ 0.f, 0.f, 0.f };
    vec3 upperLeft = vec3{ 0.f, 0.f, 0.f };
    vec3 upperRight = vec3{ 0.f, 0.f, 0.f };
};
SGCT_EXPORT void validateProjectionPlane(const ProjectionPlane& proj);



using Projections = std::variant<NoProjection, CylindricalProjection,
    EquirectangularProjection, FisheyeProjection, PlanarProjection, ProjectionPlane,
    SphericalMirrorProjection, SpoutOutputProjection, SpoutFlatProjection,
    TextureMappedProjection>;



struct SGCT_EXPORT Viewport {
    enum class Eye { Mono, StereoLeft, StereoRight };

    std::optional<std::string> user;
    std::optional<std::string> overlayTexture;
    std::optional<std::string> blendMaskTexture;
    std::optional<std::string> blackLevelMaskTexture;
    std::optional<std::string> correctionMeshTexture;
    std::optional<bool> isTracked;
    std::optional<Eye> eye;
    std::optional<vec2> position;
    std::optional<vec2> size;

    Projections projection;
};
SGCT_EXPORT void validateViewport(const Viewport& viewport, bool draw3D);



struct SGCT_EXPORT Window {
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

    int id = 0;
    std::optional<std::string> name;
    std::vector<std::string> tags;
    std::optional<ColorBitDepth> bufferBitDepth;
    std::optional<bool> isFullScreen;
    std::optional<bool> shouldAutoiconify;
    std::optional<bool> hideMouseCursor;
    std::optional<bool> isFloating;
    std::optional<bool> alwaysRender;
    std::optional<bool> isHidden;
    std::optional<bool> doubleBuffered;
    std::optional<int> msaa;
    std::optional<bool> useFxaa;
    std::optional<bool> isDecorated;
    std::optional<bool> isResizable;
    std::optional<bool> draw2D;
    std::optional<bool> draw3D;
    std::optional<bool> isMirrored;
    std::optional<int> blitWindowId;
    std::optional<int> monitor;
    std::optional<StereoMode> stereo;
    std::optional<ivec2> pos;
    ivec2 size = ivec2{ 1, 1 };
    std::optional<ivec2> resolution;

    std::vector<Viewport> viewports;
};
SGCT_EXPORT void validateWindow(const Window& window);



struct SGCT_EXPORT Node {
    std::string address;
    int port = 0;
    std::optional<int> dataTransferPort;
    std::optional<bool> swapLock;
    std::vector<Window> windows;
};
SGCT_EXPORT void validateNode(const Node& node);



struct SGCT_EXPORT Cluster {
    bool success = false;

    std::string masterAddress;
    std::optional<bool> debugLog;
    std::optional<int> setThreadAffinity;
    std::optional<bool> firmSync;
    std::optional<Scene> scene;
    std::vector<Node> nodes;
    std::vector<User> users;
    std::optional<Capture> capture;
    std::vector<Tracker> trackers;
    std::optional<Settings> settings;
};
SGCT_EXPORT void validateCluster(const Cluster& cluster);

struct SGCT_EXPORT GeneratorVersion {
    std::string name;
    int major;
    int minor;

    bool operator==(const GeneratorVersion& rhs) const {
        return (name == rhs.name) && (major == rhs.major) && (minor == rhs.minor);
    }

    bool versionCheck(GeneratorVersion check) const {
        if (check.name != name) {
            return false;
        }
        else if (major > check.major) {
            return true;
        }
        else {
            return ((major == check.major) && (minor >= check.minor));
        }
    }

    std::string versionString() const {
        return (name + " " + std::to_string(major) + "." + std::to_string(minor));
    }
};
SGCT_EXPORT void validateGeneratorVersion(const GeneratorVersion& gVersion);

struct SGCT_EXPORT Meta {
    std::string author;
    std::string description;
    std::string license;
    std::string name;
    std::string version;
};

} // namespace sgct::config

#endif // __SGCT__CONFIG__H__
