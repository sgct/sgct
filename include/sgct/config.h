/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CONFIG__H__
#define __SGCT__CONFIG__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace sgct::config {

struct SGCT_EXPORT User {
    struct Tracking {
        std::string tracker;
        std::string device;

        auto operator<=>(const Tracking&) const noexcept = default;
    };

    std::optional<std::string> name;
    std::optional<float> eyeSeparation;
    std::optional<vec3> position;
    std::optional<mat4> transformation;
    std::optional<Tracking> tracking;

    auto operator<=>(const User&) const noexcept = default;
};
SGCT_EXPORT void validateUser(const User& user);



struct SGCT_EXPORT Capture {
    struct ScreenShotRange {
        int first = -1; // inclusive
        int last = -1;  // exclusive

        auto operator<=>(const ScreenShotRange&) const noexcept = default;
    };

    std::optional<std::filesystem::path> path;
    std::optional<ScreenShotRange> range;

    auto operator<=>(const Capture&) const noexcept = default;
};
SGCT_EXPORT void validateCapture(const Capture& capture);



struct SGCT_EXPORT Scene {
    std::optional<vec3> offset;
    std::optional<quat> orientation;
    std::optional<float> scale;

    auto operator<=>(const Scene&) const noexcept = default;
};
SGCT_EXPORT void validateScene(const Scene& scene);



struct SGCT_EXPORT Settings {
    enum class BufferFloatPrecision { Float16Bit, Float32Bit};

    struct Display {
        std::optional<int8_t> swapInterval;
        std::optional<int> refreshRate;

        auto operator<=>(const Display&) const noexcept = default;
    };

    std::optional<bool> useDepthTexture;
    std::optional<bool> useNormalTexture;
    std::optional<bool> usePositionTexture;
    std::optional<BufferFloatPrecision> bufferFloatPrecision;
    std::optional<Display> display;

    auto operator<=>(const Settings&) const noexcept = default;
};
SGCT_EXPORT void validateSettings(const Settings& settings);



struct SGCT_EXPORT Tracker {
    struct Device {
        struct Sensor {
            std::string vrpnAddress;
            int identifier = -1;

            auto operator<=>(const Sensor&) const noexcept = default;
        };
        struct Button {
            std::string vrpnAddress;
            int count = 0;

            auto operator<=>(const Button&) const noexcept = default;
        };
        struct Axis {
            std::string vrpnAddress;
            int count = 0;

            auto operator<=>(const Axis&) const noexcept = default;
        };

        std::string name;
        std::vector<Sensor> sensors;
        std::vector<Button> buttons;
        std::vector<Axis> axes;
        std::optional<vec3> offset;
        std::optional<mat4> transformation;

        auto operator<=>(const Device&) const noexcept = default;
    };

    std::string name;
    std::vector<Device> devices;
    std::optional<vec3> offset;
    std::optional<double> scale;
    std::optional<mat4> transformation;

    auto operator<=>(const Tracker&) const noexcept = default;
};
SGCT_EXPORT void validateTracker(const Tracker& tracker);



struct SGCT_EXPORT NoProjection {
    auto operator<=>(const NoProjection&) const noexcept = default;
};



struct SGCT_EXPORT CubemapProjection {
    struct Channels {
        bool right = true;
        bool zLeft = true;
        bool bottom = true;
        bool top = true;
        bool left = true;
        bool zRight = true;

        auto operator<=>(const Channels&) const noexcept = default;
    };

    struct Spout {
        bool enabled = true;
        std::optional<std::string> name;

        auto operator<=>(const Spout&) const noexcept = default;
    };

    struct NDI {
        bool enabled = true;
        std::optional<std::string> name;
        std::optional<std::string> groups;

        auto operator<=>(const NDI&) const noexcept = default;
    };

    std::optional<int> quality;
    std::optional<Spout> spout;
    std::optional<NDI> ndi;
    std::optional<Channels> channels;
    std::optional<vec3> orientation;

    auto operator<=>(const CubemapProjection&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const CubemapProjection& proj);



struct SGCT_EXPORT CylindricalProjection {
    std::optional<int> quality;
    std::optional<float> rotation;
    std::optional<float> heightOffset;
    std::optional<float> radius;

    auto operator<=>(const CylindricalProjection&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const CylindricalProjection& proj);



struct SGCT_EXPORT EquirectangularProjection {
    std::optional<int> quality;

    auto operator<=>(const EquirectangularProjection&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const EquirectangularProjection& proj);



struct SGCT_EXPORT FisheyeProjection {
    enum class Interpolation { Linear, Cubic };
    struct Crop {
        // Crop values are measured as [0,1] from the respective side, which is why right
        // and top are 0 and not 1 as one might expect
        float left = 0.f;
        float right = 0.f;
        float bottom = 0.f;
        float top = 0.f;

        auto operator<=>(const Crop&) const noexcept = default;
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

    auto operator<=>(const FisheyeProjection&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const FisheyeProjection& proj);



struct SGCT_EXPORT PlanarProjection {
    struct FOV {
        float down = 0.f;
        float left = 0.f;
        float right = 0.f;
        float up = 0.f;
        std::optional<float> distance;

        auto operator<=>(const FOV&) const noexcept = default;
    };
    FOV fov;
    std::optional<quat> orientation;
    std::optional<vec3> offset;

    auto operator<=>(const PlanarProjection&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const PlanarProjection& proj);



struct SGCT_EXPORT ProjectionPlane {
    vec3 lowerLeft = vec3{ 0.f, 0.f, 0.f };
    vec3 upperLeft = vec3{ 0.f, 0.f, 0.f };
    vec3 upperRight = vec3{ 0.f, 0.f, 0.f };

    auto operator<=>(const ProjectionPlane&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const ProjectionPlane& proj);



struct SGCT_EXPORT SphericalMirrorProjection {
    struct Mesh {
        std::string bottom;
        std::string left;
        std::string right;
        std::string top;

        auto operator<=>(const Mesh&) const noexcept = default;
    };
    std::optional<int> quality;
    std::optional<float> tilt;
    std::optional<vec4> background;
    Mesh mesh;

    auto operator<=>(const SphericalMirrorProjection&) const noexcept = default;
};
SGCT_EXPORT void validateProjection(const SphericalMirrorProjection& proj);



struct SGCT_EXPORT TextureMappedProjection : PlanarProjection {
};
SGCT_EXPORT void validateProjection(const TextureMappedProjection& proj);



using Projections = std::variant<NoProjection, CubemapProjection, CylindricalProjection,
    EquirectangularProjection, FisheyeProjection, PlanarProjection, ProjectionPlane,
    SphericalMirrorProjection, TextureMappedProjection>;



struct SGCT_EXPORT Viewport {
    enum class Eye { Mono, StereoLeft, StereoRight };

    std::optional<vec2> position;
    std::optional<vec2> size;
    Projections projection = NoProjection();
    std::optional<std::filesystem::path> overlayTexture;
    std::optional<std::filesystem::path> blendMaskTexture;
    std::optional<std::filesystem::path> blackLevelMaskTexture;
    std::optional<std::filesystem::path> correctionMeshTexture;
    std::optional<bool> isTracked;
    std::optional<Eye> eye;
    std::optional<std::string> user;


    auto operator<=>(const Viewport&) const noexcept = default;
};
SGCT_EXPORT void validateViewport(const Viewport& viewport);



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

    struct Spout {
        bool enabled = true;
        std::optional<std::string> name;

        auto operator<=>(const Spout&) const noexcept = default;
    };

    struct NDI {
        bool enabled = true;
        std::optional<std::string> name;
        std::optional<std::string> groups;

        auto operator<=>(const NDI&) const noexcept = default;
    };

    struct Scalable {
        std::filesystem::path mesh;
        std::optional<int> orthographicQuality;
        std::optional<int> orthographicResolution;

        auto operator<=>(const Scalable&) const noexcept = default;
    };

    std::optional<ivec2> pos;
    ivec2 size = ivec2{ 1, 1 };
    std::optional<ivec2> resolution;
    std::vector<Viewport> viewports;
    int8_t id = 0;
    std::optional<std::string> name;
    std::vector<std::string> tags;
    std::optional<ColorBitDepth> bufferBitDepth;
    std::optional<bool> isFullScreen;
    std::optional<bool> shouldAutoiconify;
    std::optional<bool> hideMouseCursor;
    std::optional<bool> isFloating;
    std::optional<bool> alwaysRender;
    std::optional<bool> isHidden;
    std::optional<bool> takeScreenshot;
    std::optional<bool> alpha;
    std::optional<uint8_t> msaa;
    std::optional<bool> useFxaa;
    std::optional<bool> isDecorated;
    std::optional<bool> isResizable;
    std::optional<bool> draw2D;
    std::optional<bool> draw3D;
    std::optional<bool> noError;
    std::optional<int8_t> blitWindowId;
    std::optional<bool> mirrorX;
    std::optional<bool> mirrorY;
    std::optional<uint8_t> monitor;
    std::optional<StereoMode> stereo;
    std::optional<Spout> spout;
    std::optional<NDI> ndi;
    std::optional<Scalable> scalable;

    auto operator<=>(const Window&) const noexcept = default;
};
SGCT_EXPORT void validateWindow(const Window& window);



struct SGCT_EXPORT Node {
    std::string address;
    uint16_t port = 0;
    std::optional<uint16_t> dataTransferPort;
    std::optional<bool> swapLock;
    std::vector<Window> windows;

    auto operator<=>(const Node&) const noexcept = default;
};
SGCT_EXPORT void validateNode(const Node& node);



struct SGCT_EXPORT GeneratorVersion {
    std::string name;
    int major = 0;
    int minor = 0;

    auto operator<=>(const GeneratorVersion&) const noexcept = default;

    bool versionCheck(GeneratorVersion check) const;

    std::string versionString() const;
};
SGCT_EXPORT void validateGeneratorVersion(const GeneratorVersion& gVersion);



struct SGCT_EXPORT Meta {
    std::optional<std::string> author;
    std::optional<std::string> description;
    std::optional<std::string> license;
    std::optional<std::string> name;
    std::optional<std::string> version;

    auto operator<=>(const Meta&) const noexcept = default;
};



struct SGCT_EXPORT Cluster {
    bool success = false;

    std::string masterAddress;
    std::optional<bool> debugLog;
    std::optional<int> threadAffinity;
    std::optional<bool> firmSync;
    std::optional<Scene> scene;
    std::vector<Node> nodes;
    std::vector<User> users;
    std::optional<Capture> capture;
    std::vector<Tracker> trackers;
    std::optional<Settings> settings;

    std::optional<GeneratorVersion> generator;
    std::optional<Meta> meta;

    auto operator<=>(const Cluster&) const noexcept = default;
};
SGCT_EXPORT void validateCluster(const Cluster& cluster);

} // namespace sgct::config

namespace sgct {

/**
 * Reads a JSON configuration file from the provided \p filename. If the loading fails an
 * exception is raised, otherwise a valid #Cluster object is returned.
 */
SGCT_EXPORT [[nodiscard]] config::Cluster readConfig(
    const std::filesystem::path& filename);

/**
 * Reads a JSON-formatted configuration direction from the provided \p configuration. If
 * the loading fails an exception is raised, otherwise a valid #Cluster object is
 * returned.
 */
SGCT_EXPORT [[nodiscard]] config::Cluster readJsonConfig(std::string_view configuration);

SGCT_EXPORT [[nodiscard]] config::Cluster defaultCluster();

/**
 * Serialize the provided \p cluster into its JSON string representation such that parsing
 * the serialized string later with #readJsonConfig will result int the same \p cluster
 * again.
 */
SGCT_EXPORT [[nodiscard]] std::string serializeConfig(const config::Cluster& cluster,
    std::optional<config::GeneratorVersion> genVersion = std::nullopt);

/**
 * Validate the provided JSON-based string representation of a configuration against the
 * schema file located at the provided \p schema file. If the JSON is valid according to
 * the schema, the function returns the empty string. Otherwise it contains an error
 * message that describes which parts of the JSON are ill-formed.
 */
SGCT_EXPORT [[nodiscard]] std::string validateConfigAgainstSchema(
    std::string_view configuration, const std::filesystem::path& schema);

} // namespace sgct

#endif // __SGCT__CONFIG__H__
