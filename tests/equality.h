/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/config.h>
#include <sgct/math.h>
#include <nlohmann/json.hpp>

// A bunch of functions that can all go die once we move to @CPP20
namespace sgct {

bool operator==(const ivec2& lhs, const ivec2& rhs);
bool operator==(const ivec3 lhs, const ivec3& rhs);
bool operator==(const ivec4& lhs, const ivec4& rhs);
bool operator==(const vec2& lhs, const vec2& rhs);
bool operator==(const vec3& lhs, const vec3& rhs);
bool operator==(const vec4& lhs, const vec4& rhs);
bool operator==(const quat& lhs, const quat& rhs);
bool operator==(const mat4& lhs, const mat4& rhs);

namespace config {

bool operator==(const User::Tracking& lhs, const User::Tracking& rhs);
bool operator==(const User& lhs, const User& rhs);
bool operator==(const Capture::ScreenShotRange& lhs, const Capture::ScreenShotRange& rhs);
bool operator==(const Capture& lhs, const Capture& rhs);
bool operator==(const Scene& lhs, const Scene& rhs);
bool operator==(const Settings::Display& lhs, const Settings::Display& rhs);
bool operator==(const Settings& lhs, const Settings& rhs);
bool operator==(const Device::Sensors& lhs, const Device::Sensors& rhs);
bool operator==(const Device::Buttons& lhs, const Device::Buttons& rhs);
bool operator==(const Device::Axes& lhs, const Device::Axes& rhs);
bool operator==(const Device& lhs, const Device& rhs);
bool operator==(const Tracker& lhs, const Tracker& rhs);
bool operator==(const NoProjection& lhs, const NoProjection& rhs);
bool operator==(const PlanarProjection& lhs, const PlanarProjection& rhs);
bool operator==(const FisheyeProjection::Crop& lhs, const FisheyeProjection::Crop& rhs);
bool operator==(const FisheyeProjection& lhs, const FisheyeProjection& rhs);
bool operator==(const SphericalMirrorProjection& lhs,
    const SphericalMirrorProjection& rhs);
bool operator==(const SpoutOutputProjection::Channels& lhs,
    const SpoutOutputProjection::Channels& rhs);
bool operator==(const SpoutOutputProjection& lhs, const SpoutOutputProjection& rhs);
bool operator==(const CylindricalProjection& lhs, const CylindricalProjection& rhs);
bool operator==(const EquirectangularProjection& lhs,
    const EquirectangularProjection& rhs);
bool operator==(const ProjectionPlane& lhs, const ProjectionPlane& rhs);
bool operator==(const MpcdiProjection::Frustum& lhs, const MpcdiProjection::Frustum& rhs);
bool operator==(const MpcdiProjection& lhs, const MpcdiProjection& rhs);
bool operator==(const Viewport& lhs, const Viewport& rhs);
bool operator==(const Window& lhs, const Window& rhs);
bool operator==(const Node& lhs, const Node& rhs);
bool operator==(const Cluster& lhs, const Cluster& rhs);

} // namespace config
} // namespace sgct
