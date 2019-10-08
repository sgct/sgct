/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/Viewport.h>

#include <sgct/ClusterManager.h>
#include <sgct/config.h>
#include <sgct/FisheyeProjection.h>
#include <sgct/MessageHandler.h>
#include <sgct/ReadConfig.h>
#include <sgct/ScreenCapture.h>
#include <sgct/SphericalMirrorProjection.h>
#include <sgct/SpoutOutputProjection.h>
#include <sgct/TextureManager.h>
#include <algorithm>
#include <array>
#include <optional>
#include <variant>

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


} // namespace

namespace sgct::core {

Viewport::~Viewport() {
    glDeleteTextures(1, &_overlayTextureIndex);
    glDeleteTextures(1, &_blendMaskTextureIndex);
    glDeleteTextures(1, &_blackLevelMaskTextureIndex);
}

void Viewport::applySettings(const config::Viewport& viewport) {
    if (viewport.user) {
        setUserName(*viewport.user);
    }
    if (viewport.name) {
        setName(*viewport.name);
    }
    if (viewport.overlayTexture) {
        setOverlayTexture(*viewport.overlayTexture);
    }
    if (viewport.blendMaskTexture) {
        setBlendMaskTexture(*viewport.blendMaskTexture);
    }
    if (viewport.blendLevelMaskTexture) {
        setBlackLevelMaskTexture(*viewport.blendLevelMaskTexture);
    }
    if (viewport.correctionMeshTexture) {
        setCorrectionMesh(*viewport.correctionMeshTexture);
    }
    if (viewport.meshHint) {
        _meshHint = *viewport.meshHint;
    }
    if (viewport.isTracked) {
        setTracked(*viewport.isTracked);
    }
    if (viewport.eye) {
        Frustum::Mode e = [](config::Viewport::Eye e) {
            switch (e) {
                default:
                case config::Viewport::Eye::Mono:
                    return Frustum::Mode::MonoEye;
                case config::Viewport::Eye::StereoLeft:
                    return Frustum::Mode::StereoLeftEye;
                case config::Viewport::Eye::StereoRight:
                    return Frustum::Mode::StereoRightEye;
            }
        }(*viewport.eye);
        setEye(e);
    }

    if (viewport.position) {
        setPos(*viewport.position);
    }
    if (viewport.size) {
        setSize(*viewport.size);
    }

    std::visit(overloaded {
        [](const config::NoProjection&) {},
        [this](const config::PlanarProjection& proj) {
            applyPlanarProjection(proj);
        },
        [this](const config::FisheyeProjection& proj) {
            applyFisheyeProjection(proj);
        },
        [this](const config::SphericalMirrorProjection& proj) {
            applySphericalMirrorProjection(proj);
        },
        [this](const config::SpoutOutputProjection& proj) {
            applySpoutOutputProjection(proj);
        },
        [this](const config::ProjectionPlane& proj) {
            _projectionPlane.setCoordinateLowerLeft(*proj.lowerLeft);
            _viewPlane.lowerLeft = *proj.lowerLeft;
            _projectionPlane.setCoordinateUpperLeft(*proj.upperLeft);
            _viewPlane.upperLeft = *proj.upperLeft;
            _projectionPlane.setCoordinateUpperRight(*proj.upperRight);
            _viewPlane.upperRight = *proj.upperRight;
        },
    }, viewport.projection);
}

void Viewport::applySettings(const sgct::config::MpcdiProjection& proj) {
    if (proj.id) {
        setName(*proj.id);
    }
    if (proj.position) {
        setPos(*proj.position);
    }
    if (proj.size) {
        setSize(*proj.size);
    }
    if (proj.frustum) {
        setViewPlaneCoordsUsingFOVs(
            proj.frustum->up,
            proj.frustum->down,
            proj.frustum->left,
            proj.frustum->right,
            *proj.orientation,
            *proj.distance
        );
        _projectionPlane.offset(*proj.offset);
    }
}

void Viewport::applyPlanarProjection(const config::PlanarProjection& proj) {
    setViewPlaneCoordsUsingFOVs(
        proj.fov.up,
        proj.fov.down,
        proj.fov.left,
        proj.fov.right,
        proj.orientation.value_or(glm::quat()),
        proj.fov.distance.value_or(10.f)
    );
    if (proj.offset) {
        _projectionPlane.offset(*proj.offset);
    }
}

void Viewport::applyFisheyeProjection(const config::FisheyeProjection& proj) {
    std::unique_ptr<FisheyeProjection> fishProj = std::make_unique<FisheyeProjection>();
    fishProj->setUser(_user);
    
    if (proj.fov) {
        fishProj->setFOV(*proj.fov);
    }
    if (proj.quality) {
        fishProj->setCubemapResolution(*proj.quality);
    }
    if (proj.method) {
        core::FisheyeProjection::FisheyeMethod m = [](config::FisheyeProjection::Method m)
        {
            switch (m) {
                default:
                case config::FisheyeProjection::Method::FourFace:
                    return core::FisheyeProjection::FisheyeMethod::FourFaceCube;
                case config::FisheyeProjection::Method::FiveFace:
                    return core::FisheyeProjection::FisheyeMethod::FiveFaceCube;
            }
        }(*proj.method);
        fishProj->setRenderingMethod(m);
    }
    if (proj.interpolation) {
        core::NonLinearProjection::InterpolationMode i =
            [](config::FisheyeProjection::Interpolation i) {
                switch (i) {
                    default:
                    case config::FisheyeProjection::Interpolation::Linear:
                        return core::NonLinearProjection::InterpolationMode::Linear;
                    case config::FisheyeProjection::Interpolation::Cubic:
                        return core::NonLinearProjection::InterpolationMode::Cubic;
                }
            }(*proj.interpolation);
        fishProj->setInterpolationMode(i);
    }
    if (proj.tilt) {
        fishProj->setTilt(*proj.tilt);
    }
    if (proj.diameter) {
        fishProj->setDomeDiameter(*proj.diameter);
    }
    if (proj.crop) {
        fishProj->setCropFactors(
            proj.crop->left,
            proj.crop->right,
            proj.crop->bottom,
            proj.crop->top
        );
    }
    if (proj.offset) {
        fishProj->setBaseOffset(*proj.offset);
    }
    if (proj.background) {
        fishProj->setClearColor(*proj.background);
    }
    fishProj->setUseDepthTransformation(true);
    _nonLinearProjection = std::move(fishProj);
}

void Viewport::applySpoutOutputProjection(const config::SpoutOutputProjection& proj) {
#ifndef SGCT_HAS_SPOUT
    (void)proj;
    MessageHandler::instance()->print(
        MessageHandler::Level::Warning,
        "ReadConfig: Spout library not added to SGCT"
    );
    return;
#else
   
    std::unique_ptr<SpoutOutputProjection> spoutProj =
        std::make_unique<SpoutOutputProjection>();
    spoutProj->setUser(_user);
    if (proj.quality) {
        spoutProj->setCubemapResolution(*proj.quality);
    }
    if (proj.mapping) {
        SpoutOutputProjection::Mapping m = [](config::SpoutOutputProjection::Mapping m) {
            switch (m) {
                default:
                case config::SpoutOutputProjection::Mapping::Fisheye:
                    return SpoutOutputProjection::Mapping::Fisheye;
                case config::SpoutOutputProjection::Mapping::Equirectangular:
                    return SpoutOutputProjection::Mapping::Equirectangular;
                case config::SpoutOutputProjection::Mapping::Cubemap:
                    return SpoutOutputProjection::Mapping::Cubemap;
            }
        }(*proj.mapping);
        spoutProj->setSpoutMapping(m);
    }
    spoutProj->setSpoutMappingName(proj.mappingSpoutName);
    if (proj.background) {
        spoutProj->setClearColor(*proj.background);
    }
    if (proj.channels) {
        bool channels[6] = {
            proj.channels->right,
            proj.channels->zLeft,
            proj.channels->bottom,
            proj.channels->top,
            proj.channels->left,
            proj.channels->zRight
        };
        spoutProj->setSpoutChannels(channels);
    }
    if (proj.orientation) {
        spoutProj->setSpoutRigOrientation(*proj.orientation);
    }

    spoutProj->setUseDepthTransformation(true);
    _nonLinearProjection = std::move(spoutProj);
#endif
}

void Viewport::applySphericalMirrorProjection(
                                            const config::SphericalMirrorProjection& proj)
{
    std::unique_ptr<SphericalMirrorProjection> sphericalMirrorProj =
        std::make_unique<SphericalMirrorProjection>();

    sphericalMirrorProj->setUser(_user);
    if (proj.quality) {
        sphericalMirrorProj->setCubemapResolution(*proj.quality);
    }

    if (proj.tilt) {
        sphericalMirrorProj->setTilt(*proj.tilt);
    }
    if (proj.background) {
        sphericalMirrorProj->setClearColor(*proj.background);
    }

    sphericalMirrorProj->setMeshPaths(
        proj.mesh.bottom,
        proj.mesh.left,
        proj.mesh.right,
        proj.mesh.top
    );
    sphericalMirrorProj->setUseDepthTransformation(false);
    _nonLinearProjection = std::move(sphericalMirrorProj);
}

void Viewport::setOverlayTexture(std::string texturePath) {
    _overlayFilename = std::move(texturePath);
}

void Viewport::setBlendMaskTexture(std::string texturePath) {
    _blendMaskFilename = std::move(texturePath);
}

void Viewport::setBlackLevelMaskTexture(std::string texturePath) {
    _blackLevelMaskFilename = std::move(texturePath);
}

void Viewport::setCorrectionMesh(std::string meshPath) {
    _meshFilename = std::move(meshPath);
}

void Viewport::setMpcdiWarpMesh(std::vector<unsigned char> data) {
    _mpcdiWarpMesh = std::move(data);
}

void Viewport::setTracked(bool state) {
    _tracked = state;
}

void Viewport::loadData() {
    MessageHandler::instance()->printDebug(
        "Viewport: loading GPU data for '%s'", _name.c_str()
    );
        
    if (!_overlayFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            _overlayTextureIndex,
            _overlayFilename,
            true,
            1
        );
    }

    if (!_blendMaskFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            _blendMaskTextureIndex,
            _blendMaskFilename,
            true,
            1
        );
    }

    if (!_blackLevelMaskFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            _blackLevelMaskTextureIndex,
            _blackLevelMaskFilename,
            true,
            1
        );
    }

    if (!_mpcdiWarpMesh.empty()) {
        _correctionMesh = _mesh.readAndGenerateMesh(
            "mesh.mpcdi",
            *this,
            CorrectionMesh::parseHint("mpcdi")
        );
    }
    else {
        // load default if _meshFilename is empty
        _correctionMesh = _mesh.readAndGenerateMesh(
            _meshFilename,
            *this,
            CorrectionMesh::parseHint(_meshHint)
        );
    }
}

void Viewport::renderQuadMesh() const {
    if (_enabled) {
        _mesh.renderQuadMesh();
    }
}

void Viewport::renderWarpMesh() const {
    if (_enabled) {
        _mesh.renderWarpMesh();
    }
}

void Viewport::renderMaskMesh() const {
    if (_enabled) {
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

bool Viewport::hasCorrectionMesh() const {
    return _correctionMesh;
}

bool Viewport::isTracked() const {
    return _tracked;
}

unsigned int Viewport::getOverlayTextureIndex() const {
    return _overlayTextureIndex;
}

unsigned int Viewport::getBlendMaskTextureIndex() const {
    return _blendMaskTextureIndex;
}

unsigned int Viewport::getBlackLevelMaskTextureIndex() const {
    return _blackLevelMaskTextureIndex;
}

CorrectionMesh& Viewport::getCorrectionMeshPtr() {
    return _mesh;
}

NonLinearProjection* Viewport::getNonLinearProjection() const {
    return _nonLinearProjection.get();
}

const std::vector<unsigned char>& Viewport::mpcdiWarpMesh() const {
    return _mpcdiWarpMesh;
}

} // namespace sgct::core
