/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/viewport.h>

#include <sgct/clustermanager.h>
#include <sgct/config.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/readconfig.h>
#include <sgct/screencapture.h>
#include <sgct/texturemanager.h>
#include <sgct/projection/cylindrical.h>
#include <sgct/projection/equirectangular.h>
#include <sgct/projection/fisheye.h>
#include <sgct/projection/nonlinearprojection.h>
#include <sgct/projection/sphericalmirror.h>
#include <sgct/projection/spout.h>
#include <sgct/projection/spoutflat.h>
#include <algorithm>
#include <array>
#include <optional>
#include <variant>

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace

namespace sgct {

Viewport::Viewport(const Window* parent) : BaseViewport(parent) {}

Viewport::~Viewport() = default;

void Viewport::initialize(vec2 size, bool hasStereo, unsigned int internalFormat,
                          unsigned int format, unsigned int type, int samples)
{
    if (_nonLinearProjection) {
        _nonLinearProjection->setStereo(hasStereo);
        _nonLinearProjection->initialize(internalFormat, format, type, samples);
        _nonLinearProjection->update(std::move(size));
    }
}

void Viewport::applyViewport(const config::Viewport& viewport) {
    ZoneScoped;

    if (viewport.user) {
        setUserName(*viewport.user);
    }
    if (viewport.overlayTexture) {
        _overlayFilename = *viewport.overlayTexture;
    }
    if (viewport.blendMaskTexture) {
        _blendMaskFilename = *viewport.blendMaskTexture;
    }
    if (viewport.blackLevelMaskTexture) {
        _blackLevelMaskFilename = *viewport.blackLevelMaskTexture;
    }
    if (viewport.correctionMeshTexture) {
        _meshFilename = *viewport.correctionMeshTexture;
    }
    if (viewport.isTracked) {
        _isTracked = *viewport.isTracked;
    }
    if (viewport.eye) {
        Frustum::Mode eye = [](config::Viewport::Eye e) {
            using Mode = Frustum::Mode;
            switch (e) {
                case config::Viewport::Eye::Mono: return Mode::MonoEye;
                case config::Viewport::Eye::StereoLeft: return Mode::StereoLeftEye;
                case config::Viewport::Eye::StereoRight: return Mode::StereoRightEye;
                default: throw std::logic_error("Unhandled case label");
            }
        }(*viewport.eye);
        setEye(eye);
    }

    if (viewport.position) {
        setPos(*viewport.position);
    }
    if (viewport.size) {
        setSize(*viewport.size);
    }

    std::visit(overloaded {
        [](const config::NoProjection&) {},
        [this](const config::PlanarProjection& p) { applyPlanarProjection(p); },
        [this](const config::TextureMappedProjection& p) {
            _useTextureMappedProjection = true;
            applyPlanarProjection(p);
        },
        [this](const config::FisheyeProjection& p) { applyFisheyeProjection(p); },
        [this](const config::SphericalMirrorProjection& p) {
            applySphericalMirrorProjection(p);
        },
        [this](const config::SpoutOutputProjection& p) { applySpoutOutputProjection(p); },
        [this](const config::SpoutFlatProjection& p) { applySpoutFlatProjection(p); },
        [this](const config::CylindricalProjection& p) { applyCylindricalProjection(p); },
        [this](const config::EquirectangularProjection& p) {
            applyEquirectangularProjection(p);
        },
        [this](const config::ProjectionPlane& p) {
            _projPlane.setCoordinates(p.lowerLeft, p.upperLeft, p.upperRight);
            _viewPlane.lowerLeft = p.lowerLeft;
            _viewPlane.upperLeft = p.upperLeft;
            _viewPlane.upperRight = p.upperRight;
        },
    }, viewport.projection);
}

void Viewport::applyPlanarProjection(const config::PlanarProjection& proj) {
    ZoneScoped;

    setViewPlaneCoordsUsingFOVs(
        proj.fov.up,
        proj.fov.down,
        proj.fov.left,
        proj.fov.right,
        proj.orientation.value_or(quat{ 0.f, 0.f, 0.f, 1.f }),
        proj.fov.distance.value_or(10.f)
    );
    if (proj.offset) {
        _projPlane.offset(*proj.offset);
    }
}

void Viewport::applyFisheyeProjection(const config::FisheyeProjection& proj) {
    ZoneScoped;

    auto fishProj = std::make_unique<FisheyeProjection>(_parent);
    fishProj->setUser(_user);

    if (proj.fov) {
        fishProj->setFOV(*proj.fov);
    }
    if (proj.quality) {
        fishProj->setCubemapResolution(*proj.quality);
    }
    if (proj.interpolation) {
        NonLinearProjection::InterpolationMode interp =
            [](config::FisheyeProjection::Interpolation i) {
                switch (i) {
                    case config::FisheyeProjection::Interpolation::Linear:
                        return NonLinearProjection::InterpolationMode::Linear;
                    case config::FisheyeProjection::Interpolation::Cubic:
                        return NonLinearProjection::InterpolationMode::Cubic;
                    default: throw std::logic_error("Unhandled case label");
                }
            }(*proj.interpolation);
        fishProj->setInterpolationMode(interp);
    }
    if (proj.tilt) {
        fishProj->setTilt(*proj.tilt);
    }
    if (proj.diameter) {
        fishProj->setDomeDiameter(*proj.diameter);
    }
    if (proj.crop) {
        config::FisheyeProjection::Crop crop = *proj.crop;
        fishProj->setCropFactors(crop.left, crop.right, crop.bottom, crop.top);
    }
    if (proj.offset) {
        fishProj->setBaseOffset(*proj.offset);
    }
    if (proj.background) {
        fishProj->setClearColor(*proj.background);
    }
    if (proj.keepAspectRatio) {
        fishProj->setKeepAspectRatio(*proj.keepAspectRatio);
    }
    fishProj->setUseDepthTransformation(true);
    _nonLinearProjection = std::move(fishProj);
}

void Viewport::applySpoutOutputProjection(const config::SpoutOutputProjection& p) {
#ifdef SGCT_HAS_SPOUT
    ZoneScoped;

    auto proj = std::make_unique<SpoutOutputProjection>(_parent);
    proj->setUser(_user);
    if (p.quality) {
        proj->setCubemapResolution(*p.quality);
    }
    if (p.mapping) {
        SpoutOutputProjection::Mapping m = [](config::SpoutOutputProjection::Mapping m) {
            switch (m) {
                case config::SpoutOutputProjection::Mapping::Fisheye:
                    return SpoutOutputProjection::Mapping::Fisheye;
                case config::SpoutOutputProjection::Mapping::Equirectangular:
                    return SpoutOutputProjection::Mapping::Equirectangular;
                case config::SpoutOutputProjection::Mapping::Cubemap:
                    return SpoutOutputProjection::Mapping::Cubemap;
                default: throw std::logic_error("Unhandled case label");
            }
        }(*p.mapping);
        proj->setSpoutMapping(m);
    }
    proj->setSpoutMappingName(p.mappingSpoutName);
    if (p.background) {
        proj->setClearColor(*p.background);
    }
    if (p.channels) {
        config::SpoutOutputProjection::Channels c = *p.channels;
        proj->setSpoutChannels(c.right, c.zLeft, c.bottom, c.top, c.left, c.zRight);
    }
    if (p.orientation) {
        proj->setSpoutRigOrientation(*p.orientation);
    }
    if (p.drawMain) {
        proj->setSpoutDrawMain(*p.drawMain);
    }

    _nonLinearProjection = std::move(proj);
#else
    (void)p;
    Log::Warning("Spout library not added to SGCT");
#endif
}

void Viewport::applySpoutFlatProjection(const config::SpoutFlatProjection& p) {
#ifdef SGCT_HAS_SPOUT
    ZoneScoped;

    auto proj = std::make_unique<SpoutFlatProjection>(_parent);
    proj->setUser(_user);
    if (p.width) {
        proj->setResolutionWidth(*p.width);
    }
    if (p.height) {
        proj->setResolutionHeight(*p.height);
    }
    proj->setSpoutMappingName(p.mappingSpoutName);
    if (p.background) {
        proj->setClearColor(*p.background);
    }
    if (p.drawMain) {
        proj->setSpoutDrawMain(*p.drawMain);
    }

    if (p.proj.offset) {
        proj->setSpoutOffset(*p.proj.offset);
    }
    proj->setSpoutFov(p.proj.fov.up,
        p.proj.fov.down,
        p.proj.fov.left,
        p.proj.fov.right,
        p.proj.orientation.value_or(quat{ 0.f, 0.f, 0.f, 1.f }),
        p.proj.fov.distance.value_or(10.f));

    _nonLinearProjection = std::move(proj);
#else
    (void)p;
    Log::Warning("Spout library not added to SGCT");
#endif
}

void Viewport::applyCylindricalProjection(const config::CylindricalProjection& p) {
    ZoneScoped;

    auto proj = std::make_unique<CylindricalProjection>(_parent);
    proj->setUser(_user);
    if (p.quality) {
        proj->setCubemapResolution(*p.quality);
    }
    if (p.rotation) {
        proj->setRotation(*p.rotation);
    }
    if (p.heightOffset) {
        proj->setHeightOffset(*p.heightOffset);
    }
    if (p.radius) {
        proj->setRadius(*p.radius);
    }

    _nonLinearProjection = std::move(proj);
}

void Viewport::applyEquirectangularProjection(const config::EquirectangularProjection& p)
{
    ZoneScoped;

    auto proj = std::make_unique<EquirectangularProjection>(_parent);
    proj->setUser(_user);
    if (p.quality) {
        proj->setCubemapResolution(*p.quality);
    }

    _nonLinearProjection = std::move(proj);
}

void Viewport::applySphericalMirrorProjection(const config::SphericalMirrorProjection& p)
{
    ZoneScoped;

    auto proj = std::make_unique<SphericalMirrorProjection>(
        _parent,
        p.mesh.bottom,
        p.mesh.left,
        p.mesh.right,
        p.mesh.top
    );

    proj->setUser(_user);
    if (p.quality) {
        proj->setCubemapResolution(*p.quality);
    }
    if (p.tilt) {
        proj->setTilt(*p.tilt);
    }
    if (p.background) {
        proj->setClearColor(*p.background);
    }

    _nonLinearProjection = std::move(proj);
}

void Viewport::loadData() {
    ZoneScoped;

    TextureManager& mgr = TextureManager::instance();
    if (!_overlayFilename.empty()) {
        _overlayTextureIndex = mgr.loadTexture(_overlayFilename, true, 1);
    }

    if (!_blendMaskFilename.empty()) {
        _blendMaskTextureIndex = mgr.loadTexture(_blendMaskFilename, true, 1);
    }

    if (!_blackLevelMaskFilename.empty()) {
        _blackLevelMaskTextureIndex = mgr.loadTexture(_blackLevelMaskFilename, true, 1);
    }

    _mesh.loadMesh(
        _meshFilename,
        *this,
        (hasBlendMaskTexture() || hasBlackLevelMaskTexture()),
        _useTextureMappedProjection
    );
}

void Viewport::renderQuadMesh() const {
    ZoneScoped;

    if (_isEnabled) {
        _mesh.renderQuadMesh();
    }
}

void Viewport::renderWarpMesh() const {
    ZoneScoped;

    if (_isEnabled) {
        _mesh.renderWarpMesh();
    }
}

void Viewport::renderMaskMesh() const {
    ZoneScoped;

    if (_isEnabled) {
        _mesh.renderMaskMesh();
    }
}

bool Viewport::hasOverlayTexture() const {
    return _overlayTextureIndex != 0;
}

bool Viewport::hasBlendMaskTexture() const {
    return _blendMaskTextureIndex != 0;
}

bool Viewport::hasBlackLevelMaskTexture() const {
    return _blackLevelMaskTextureIndex != 0;
}

bool Viewport::hasSubViewports() const {
    return _nonLinearProjection != nullptr;
}

bool Viewport::isTracked() const {
    return _isTracked;
}

unsigned int Viewport::overlayTextureIndex() const {
    return _overlayTextureIndex;
}

unsigned int Viewport::blendMaskTextureIndex() const {
    return _blendMaskTextureIndex;
}

unsigned int Viewport::blackLevelMaskTextureIndex() const {
    return _blackLevelMaskTextureIndex;
}

NonLinearProjection* Viewport::nonLinearProjection() const {
    return _nonLinearProjection.get();
}

} // namespace sgct
