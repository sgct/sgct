/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

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
    std::optional<std::string> name;
    std::optional<float> eyeSeparation;
    std::optional<glm::vec3> position;
    std::optional<glm::quat> orientation;
    struct Transform {
        glm::mat4 transformation;
        bool transpose = false;
    };
    std::optional<Transform> transformation;
    struct Tracking {
        std::string tracker;
        std::string device;
    };
    std::optional<Tracking> tracking;
};
bool validateUser(const User& user);

struct Capture {
    enum class Format { PNG, JPG, TGA };
    std::optional<std::string> monoPath;
    std::optional<std::string> leftPath;
    std::optional<std::string> rightPath;
    std::optional<Format> format;

};
bool validateCapture(const Capture& capture);

struct Scene {
    std::optional<glm::vec3> offset;
    std::optional<glm::quat> orientation;
    std::optional<float> scale;
};
bool validateScene(const Scene& scene);

struct Device {
    struct Sensors {
        std::string vrpnAddress;
        int identifier;
    };
    struct Buttons {
        std::string vrpnAddress;
        int count;
    };
    struct Axes {
        std::string vrpnAddress;
        int count;
    };
    struct Transform {
        glm::mat4 transformation;
        bool transpose = false;
    };
    
    std::string name;
    std::vector<Sensors> sensors;
    std::vector<Buttons> buttons;
    std::vector<Axes> axes;
    std::optional<glm::vec3> offset;
    std::optional<glm::quat> orientation;
    std::optional<Transform> transformation;
};
bool validateDevice(const Device& device);

struct Tracker {
    struct Transform {
        glm::mat4 transformation;
        bool transpose = false;
    };

    std::string name;
    std::vector<Device> devices;
    std::optional<glm::vec3> offset;
    std::optional<glm::quat> orientation;
    std::optional<double> scale;
    std::optional<Transform> transformation;
};
bool validateTracker(const Tracker& tracker);

struct NoProjection {};
struct PlanarProjection {
    struct FOV {
        float down;
        float left;
        float right;
        float up;
        float distance = 10.f;
    };
    std::optional<FOV> fov;
    std::optional<glm::quat> orientation;
    std::optional<glm::vec3> offset;
};
bool validatePlanarProjection(const PlanarProjection& proj);

struct FisheyeProjection {
    enum class Method { FourFace, FiveFace };
    enum class Interpolation { Linear, Cubic };
    struct Crop {
        // @TODO (abock, 2019-09-21) The default values for right and top should probably
        // be 1.f ?
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
bool validateFisheyeProjection(const FisheyeProjection& proj);

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
bool validateSphericalMirrorProjection(const SphericalMirrorProjection& proj);

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
    std::optional<std::string> mappingSpoutName;
    std::optional<glm::vec4> background;
    std::optional<Channels> channels;
    std::optional<glm::quat> orientation;
};
bool validateSpoutOutputProjection(const SpoutOutputProjection& proj);

struct ProjectionPlane {
    std::optional<glm::vec3> lowerLeft;
    std::optional<glm::vec3> upperLeft;
    std::optional<glm::vec3> upperRight;
};
bool validateProjectionPlane(const ProjectionPlane& proj);

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
bool validateMpcdiProjection(const MpcdiProjection& proj);

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

    glm::vec2 position;
    glm::vec2 size;
    std::variant<NoProjection, PlanarProjection, FisheyeProjection,
        SphericalMirrorProjection, SpoutOutputProjection, ProjectionPlane> projection;
};






struct Window {
    std::string name;
    std::vector<std::string> tags;
    std::string bufferBitDepth;
    bool preferBGR;
    std::string stereoType;
    bool isFullScreen;
    bool isFloating;
    bool alwaysRender;
    bool isHidden;
    bool doubleBuffered;
    float gamma;
    float contrast;
    float brightness;
    int msaa;
    bool hasAlpha;
    bool useFxaa;
    bool isDecorated;
    bool hasBorder;
    bool draw2D;
    bool draw3D;
    bool copyPreviousWindowToCurrentWindow;
    int monitor;
    std::string mpcdi;
    std::string stereo;
    glm::ivec2 pos;
    glm::ivec2 size;
    glm::ivec2 resolution;

    std::vector<Viewport> viewports;
};

struct Node {
    std::string address;
    int port;
    std::vector<Window> windows;
};


struct Cluster {
    std::string masterAddress;
    std::vector<Node> nodes;
    User user;
};

} // namespace sgct::config

#endif // __SGCT__CONFIG__H__
