/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "equality.h"

namespace sgct {

bool operator==(const ivec2& lhs, const ivec2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator==(const ivec3 lhs, const ivec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool operator==(const ivec4& lhs, const ivec4& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

bool operator==(const vec2& lhs, const vec2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator==(const vec3& lhs, const vec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool operator==(const vec4& lhs, const vec4& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

bool operator==(const quat& lhs, const quat& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

bool operator==(const mat4& lhs, const mat4& rhs) {
    return
        lhs.values[0] == rhs.values[0] &&
        lhs.values[1] == rhs.values[1] &&
        lhs.values[2] == rhs.values[2] &&
        lhs.values[3] == rhs.values[3] &&
        lhs.values[4] == rhs.values[4] &&
        lhs.values[5] == rhs.values[5] &&
        lhs.values[6] == rhs.values[6] &&
        lhs.values[7] == rhs.values[7] &&
        lhs.values[8] == rhs.values[8] &&
        lhs.values[9] == rhs.values[9] &&
        lhs.values[10] == rhs.values[10] &&
        lhs.values[11] == rhs.values[11] &&
        lhs.values[12] == rhs.values[12] &&
        lhs.values[13] == rhs.values[13] &&
        lhs.values[14] == rhs.values[14] &&
        lhs.values[15] == rhs.values[15];
}

namespace config {

bool operator==(const User::Tracking& lhs, const User::Tracking& rhs) {
    return lhs.tracker == rhs.tracker && lhs.device == rhs.device;
}

bool operator==(const User& lhs, const User& rhs) {
    return
        lhs.name == rhs.name &&
        lhs.eyeSeparation == rhs.eyeSeparation &&
        lhs.position == rhs.position &&
        lhs.transformation == rhs.transformation &&
        lhs.tracking == rhs.tracking;
}

bool operator==(const Capture::ScreenShotRange& lhs, const Capture::ScreenShotRange& rhs)
{
    return lhs.first == rhs.first && lhs.last == rhs.last;
}

bool operator==(const Capture& lhs, const Capture& rhs) {
    return
        lhs.path == rhs.path &&
        lhs.format == rhs.format &&
        lhs.range == rhs.range;
}

bool operator==(const Scene& lhs, const Scene& rhs) {
    return
        lhs.offset == rhs.offset &&
        lhs.orientation == rhs.orientation &&
        lhs.scale == rhs.scale;
}

bool operator==(const Settings::Display& lhs, const Settings::Display& rhs) {
    return lhs.swapInterval == rhs.swapInterval && lhs.refreshRate == rhs.refreshRate;
}

bool operator==(const Settings& lhs, const Settings& rhs) {
    return
        lhs.useDepthTexture == rhs.useDepthTexture &&
        lhs.useNormalTexture == rhs.useNormalTexture &&
        lhs.usePositionTexture == rhs.usePositionTexture &&
        lhs.bufferFloatPrecision == rhs.bufferFloatPrecision &&
        lhs.display == rhs.display;
}

bool operator==(const Device::Sensors& lhs, const Device::Sensors& rhs) {
    return lhs.vrpnAddress == rhs.vrpnAddress && lhs.identifier == rhs.identifier;
}

bool operator==(const Device::Buttons& lhs, const Device::Buttons& rhs) {
    return lhs.vrpnAddress == rhs.vrpnAddress && lhs.count == rhs.count;
}

bool operator==(const Device::Axes& lhs, const Device::Axes& rhs) {
    return lhs.vrpnAddress == rhs.vrpnAddress && lhs.count == rhs.count;
}

bool operator==(const Device& lhs, const Device& rhs) {
    return
        lhs.name == rhs.name &&
        lhs.sensors == rhs.sensors &&
        lhs.buttons == rhs.buttons &&
        lhs.axes == rhs.axes &&
        lhs.offset == rhs.offset &&
        lhs.transformation == rhs.transformation;
}

bool operator==(const Tracker& lhs, const Tracker& rhs) {
    return
        lhs.name == rhs.name &&
        lhs.devices == rhs.devices &&
        lhs.offset == rhs.offset &&
        lhs.scale == rhs.scale &&
        lhs.transformation == rhs.transformation;
}

bool operator==(const NoProjection& lhs, const NoProjection& rhs) {
    return true;
}

bool operator==(const PlanarProjection& lhs, const PlanarProjection& rhs) {
    return
        lhs.fov.down == rhs.fov.down &&
        lhs.fov.left == rhs.fov.left &&
        lhs.fov.right == rhs.fov.right &&
        lhs.fov.up == rhs.fov.up &&
        lhs.fov.distance == rhs.fov.distance &&
        lhs.orientation == rhs.orientation &&
        lhs.offset == rhs.offset;
}

bool operator==(const TextureMappedProjection& lhs, const TextureMappedProjection& rhs) {
    return
        lhs.fov.down == rhs.fov.down &&
        lhs.fov.left == rhs.fov.left &&
        lhs.fov.right == rhs.fov.right &&
        lhs.fov.up == rhs.fov.up &&
        lhs.fov.distance == rhs.fov.distance &&
        lhs.orientation == rhs.orientation &&
        lhs.offset == rhs.offset;
}

bool operator==(const FisheyeProjection::Crop& lhs, const FisheyeProjection::Crop& rhs) {
    return
        lhs.left == rhs.left &&
        lhs.right == rhs.right &&
        lhs.bottom == rhs.bottom &&
        lhs.top == rhs.top;
}

bool operator==(const FisheyeProjection& lhs, const FisheyeProjection& rhs) {
    return
        lhs.fov == rhs.fov &&
        lhs.quality == rhs.quality &&
        lhs.interpolation == rhs.interpolation &&
        lhs.tilt == rhs.tilt &&
        lhs.diameter == rhs.diameter &&
        lhs.crop == rhs.crop &&
        lhs.keepAspectRatio == rhs.keepAspectRatio &&
        lhs.offset == rhs.offset &&
        lhs.background == rhs.background;
}

bool operator==(const SphericalMirrorProjection& lhs,
                const SphericalMirrorProjection& rhs)
{
    return
        lhs.quality == rhs.quality &&
        lhs.tilt == rhs.tilt &&
        lhs.background == rhs.background &&
        lhs.mesh.bottom == rhs.mesh.bottom &&
        lhs.mesh.left == rhs.mesh.left &&
        lhs.mesh.right == rhs.mesh.right &&
        lhs.mesh.top == rhs.mesh.top;
}

bool operator==(const SpoutOutputProjection::Channels& lhs,
                const SpoutOutputProjection::Channels& rhs)
{
    return
        lhs.right == rhs.right &&
        lhs.zLeft == rhs.zLeft &&
        lhs.bottom == rhs.bottom &&
        lhs.top == rhs.top &&
        lhs.left == rhs.left &&
        lhs.zRight == rhs.zRight;
}

bool operator==(const SpoutOutputProjection& lhs, const SpoutOutputProjection& rhs) {
    return
        lhs.quality == rhs.quality &&
        lhs.mapping == rhs.mapping &&
        lhs.mappingSpoutName == rhs.mappingSpoutName &&
        lhs.background == rhs.background &&
        lhs.channels == rhs.channels &&
        lhs.orientation == rhs.orientation;
}

bool operator==(const SpoutFlatProjection& lhs, const SpoutFlatProjection& rhs) {
    return
        lhs.proj == rhs.proj &&
        lhs.width == rhs.width &&
        lhs.height == rhs.height &&
        lhs.mappingSpoutName == rhs.mappingSpoutName &&
        lhs.background == rhs.background &&
        lhs.drawMain == rhs.drawMain;
}

bool operator==(const CylindricalProjection& lhs, const CylindricalProjection& rhs) {
    return
        lhs.quality == rhs.quality &&
        lhs.rotation == rhs.rotation &&
        lhs.heightOffset == rhs.heightOffset &&
        lhs.radius == rhs.radius;
}

bool operator==(const EquirectangularProjection& lhs,
                const EquirectangularProjection& rhs)
{
    return
        lhs.quality == rhs.quality;
}

bool operator==(const ProjectionPlane& lhs, const ProjectionPlane& rhs) {
    return
        lhs.lowerLeft == rhs.lowerLeft &&
        lhs.upperLeft == rhs.upperLeft &&
        lhs.upperRight == rhs.upperRight;
}

bool operator==(const Viewport& lhs, const Viewport& rhs) {
    return
        lhs.user == rhs.user &&
        lhs.overlayTexture == rhs.overlayTexture &&
        lhs.blendMaskTexture == rhs.blendMaskTexture &&
        lhs.blackLevelMaskTexture == rhs.blackLevelMaskTexture &&
        lhs.correctionMeshTexture == rhs.correctionMeshTexture &&
        lhs.isTracked == rhs.isTracked &&
        lhs.eye == rhs.eye &&
        lhs.position == rhs.position &&
        lhs.size == rhs.size &&
        lhs.projection == rhs.projection;
}

bool operator==(const Window& lhs, const Window& rhs) {
    return
        lhs.id == rhs.id &&
        lhs.name == rhs.name &&
        lhs.tags == rhs.tags &&
        lhs.bufferBitDepth == rhs.bufferBitDepth &&
        lhs.isFullScreen == rhs.isFullScreen &&
        lhs.shouldAutoiconify == rhs.shouldAutoiconify &&
        lhs.hideMouseCursor == rhs.hideMouseCursor &&
        lhs.isFloating == rhs.isFloating &&
        lhs.alwaysRender == rhs.alwaysRender &&
        lhs.isHidden == rhs.isHidden &&
        lhs.doubleBuffered == rhs.doubleBuffered &&
        lhs.msaa == rhs.msaa &&
        lhs.useFxaa == rhs.useFxaa &&
        lhs.isDecorated == rhs.isDecorated &&
        lhs.draw2D == rhs.draw2D &&
        lhs.draw3D == rhs.draw3D &&
        lhs.isMirrored == rhs.isMirrored &&
        lhs.blitWindowId == rhs.blitWindowId &&
        lhs.monitor == rhs.monitor &&
        lhs.stereo == rhs.stereo &&
        lhs.pos == rhs.pos &&
        lhs.size == rhs.size &&
        lhs.resolution == rhs.resolution &&
        lhs.viewports == rhs.viewports;
}

bool operator==(const Node& lhs, const Node& rhs) {
    return
        lhs.address == rhs.address &&
        lhs.port == rhs.port &&
        lhs.dataTransferPort == rhs.dataTransferPort &&
        lhs.swapLock == rhs.swapLock &&
        lhs.windows == rhs.windows;
}

bool operator==(const Cluster& lhs, const Cluster& rhs) {
    return
        lhs.success == rhs.success &&
        lhs.masterAddress == rhs.masterAddress &&
        lhs.debugLog == rhs.debugLog &&
        lhs.setThreadAffinity == rhs.setThreadAffinity &&
        lhs.firmSync == rhs.firmSync &&
        lhs.scene == rhs.scene &&
        lhs.nodes == rhs.nodes &&
        lhs.users == rhs.users &&
        lhs.capture == rhs.capture &&
        lhs.trackers == rhs.trackers &&
        lhs.settings == rhs.settings;
}

} // namespace config
} // namespace sgct
