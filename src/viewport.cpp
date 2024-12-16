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
#include <sgct/screencapture.h>
#include <sgct/texturemanager.h>
#include <sgct/projection/cubemap.h>
#include <sgct/projection/cylindrical.h>
#include <sgct/projection/equirectangular.h>
#include <sgct/projection/fisheye.h>
#include <sgct/projection/nonlinearprojection.h>
#include <sgct/projection/sphericalmirror.h>
#include <algorithm>
#include <array>
#include <optional>
#include <variant>

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    sgct::FrustumMode convert(sgct::config::Viewport::Eye e) {
        using namespace sgct;
        switch (e) {
            case config::Viewport::Eye::Mono:        return FrustumMode::Mono;
            case config::Viewport::Eye::StereoLeft:  return FrustumMode::StereoLeft;
            case config::Viewport::Eye::StereoRight: return FrustumMode::StereoRight;
            default:                       throw std::logic_error("Unhandled case label");
        }
    }
} // namespace

namespace sgct {

Viewport::Viewport(const config::Viewport& viewport, const Window& parent)
    : BaseViewport(parent, convert(viewport.eye.value_or(config::Viewport::Eye::Mono)))
    , _overlayFilename(viewport.overlayTexture.value_or(std::filesystem::path()))
    , _blendMaskFilename(viewport.blendMaskTexture.value_or(std::filesystem::path()))
    , _blackLevelMaskFilename(
        viewport.blackLevelMaskTexture.value_or(std::filesystem::path())
    )
    , _meshFilename(viewport.correctionMeshTexture.value_or(std::filesystem::path()))
    , _isTracked(viewport.isTracked.value_or(false))
{
    if (viewport.user) {
        User* user = ClusterManager::instance().user(*viewport.user);
        if (!user) {
            Log::Warning(
                std::format("Could not find user with name '{}'", *viewport.user)
            );
        }

        // If the user name is not empty, the User better exists
        _user = user;
    }
    assert(_user);

    _position = viewport.position.value_or(_position);
    _size = viewport.size.value_or(_size);

    std::visit(overloaded {
        [](const config::NoProjection&) {},
        [this](const config::PlanarProjection& p) {
            setViewPlaneCoordsUsingFOVs(
                p.fov.up,
                p.fov.down,
                p.fov.left,
                p.fov.right,
                p.orientation.value_or(quat{ 0.f, 0.f, 0.f, 1.f }),
                p.fov.distance.value_or(10.f)
            );
            if (p.offset) {
                _projPlane.offset(*p.offset);
            }
        },
        [this](const config::TextureMappedProjection& p) {
            _useTextureMappedProjection = true;
            setViewPlaneCoordsUsingFOVs(
                p.fov.up,
                p.fov.down,
                p.fov.left,
                p.fov.right,
                p.orientation.value_or(quat{ 0.f, 0.f, 0.f, 1.f }),
                p.fov.distance.value_or(10.f)
            );
            if (p.offset) {
                _projPlane.offset(*p.offset);
            }
        },
        [this](const config::FisheyeProjection& p) {
            _nonLinearProjection = std::make_unique<FisheyeProjection>(
                p,
                _parent,
                *_user
            );
        },
        [this](const config::SphericalMirrorProjection& p) {
            _nonLinearProjection =
                std::make_unique<SphericalMirrorProjection>(p, _parent, *_user);
        },
        [this]([[maybe_unused]] const config::CubemapProjection& p) {
            _nonLinearProjection =
                std::make_unique<CubemapProjection>(p, _parent, *_user);
        },
        [this](const config::CylindricalProjection& p) {
            _nonLinearProjection =
                std::make_unique<CylindricalProjection>(p, _parent, *_user);
        },
        [this](const config::EquirectangularProjection& p) {
            _nonLinearProjection =
                std::make_unique<EquirectangularProjection>(p, _parent, *_user);
        },
        [this](const config::ProjectionPlane& p) {
            _projPlane.setCoordinates(p.lowerLeft, p.upperLeft, p.upperRight);
            _viewPlane.lowerLeft = p.lowerLeft;
            _viewPlane.upperLeft = p.upperLeft;
            _viewPlane.upperRight = p.upperRight;
        },
    }, viewport.projection);
}

Viewport::~Viewport() = default;

void Viewport::initialize(vec2 size, bool hasStereo, unsigned int internalFormat,
                          unsigned int format, unsigned int type, uint8_t samples)
{
    if (_nonLinearProjection) {
        _nonLinearProjection->setStereo(hasStereo);
        _nonLinearProjection->initialize(internalFormat, format, type, samples);
        _nonLinearProjection->update(std::move(size));
    }
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
        hasBlendMaskTexture() || hasBlackLevelMaskTexture(),
        _useTextureMappedProjection
    );
}

void Viewport::calculateFrustum(FrustumMode mode, float nearClip, float farClip) {
    if (_nonLinearProjection) {
        _nonLinearProjection->updateFrustums(mode, nearClip, farClip);
    }
    else {
        BaseViewport::calculateFrustum(mode, nearClip, farClip);
    }
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
