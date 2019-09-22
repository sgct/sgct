/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

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

namespace {
    std::optional<float> parseFrustumElement(const tinyxml2::XMLElement& elem,
                                             std::string_view frustumTag)
    {
        if (frustumTag == elem.Value()) {
            try {
                return std::stof(elem.GetText());
            }
            catch (const std::invalid_argument&) {
                std::string msg = "Viewport: Failed to parse frustum element "
                    + std::string(frustumTag) + " from MPCDI XML\n";
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    msg.c_str()
                );
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] sgct::config::PlanarProjection parsePlanarProjection(tinyxml2::XMLElement* element){
        using namespace tinyxml2;

        sgct::config::PlanarProjection proj;
        XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "FOV") {
                sgct::config::PlanarProjection::FOV fov;
                XMLError errDown = subElement->QueryFloatAttribute("down", &fov.down);
                XMLError errLeft = subElement->QueryFloatAttribute("left", &fov.left);
                XMLError errRight = subElement->QueryFloatAttribute("right", &fov.right);
                XMLError errUp = subElement->QueryFloatAttribute("up", &fov.up);

                if (errDown == XML_NO_ERROR && errLeft == XML_NO_ERROR &&
                    errRight == XML_NO_ERROR && errUp == XML_NO_ERROR)
                {
                    proj.fov = fov;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse planar projection FOV from XML\n"
                    );
                }
            }
            else if (val == "Orientation") {
                proj.orientation = sgct::core::readconfig::parseOrientationNode(subElement);
            }
            else if (val == "Offset") {
                glm::vec3 offset;
                subElement->QueryFloatAttribute("x", &offset[0]);
                subElement->QueryFloatAttribute("y", &offset[1]);
                subElement->QueryFloatAttribute("z", &offset[2]);
                proj.offset = offset;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::FisheyeProjection parseFisheyeProjection(tinyxml2::XMLElement* element) {
        sgct::config::FisheyeProjection proj;

        float fov;
        if (element->QueryFloatAttribute("fov", &fov) == tinyxml2::XML_NO_ERROR) {
            proj.fov = fov;
        }
        if (element->Attribute("quality")) {
            const int res = sgct::core::cubeMapResolutionForQuality(element->Attribute("quality"));
            proj.quality = res;
        }
        if (element->Attribute("method")) {
            std::string_view method = element->Attribute("method");
            proj.method = method == "five_face_cube" ?
                sgct::config::FisheyeProjection::Method::FiveFace :
                sgct::config::FisheyeProjection::Method::FourFace;
        }
        if (element->Attribute("interpolation")) {
            std::string_view interpolation = element->Attribute("interpolation");
            proj.interpolation = interpolation == "cubic" ?
                sgct::config::FisheyeProjection::Interpolation::Cubic :
                sgct::config::FisheyeProjection::Interpolation::Linear;
        }
        float diameter;
        if (element->QueryFloatAttribute("diameter", &diameter) == tinyxml2::XML_NO_ERROR) {
            proj.diameter = diameter;
        }

        float tilt;
        if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR) {
            proj.tilt = tilt;
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Crop") {
                sgct::config::FisheyeProjection::Crop crop;
                subElement->QueryFloatAttribute("left", &crop.left);
                subElement->QueryFloatAttribute("right", &crop.right);
                subElement->QueryFloatAttribute("bottom", &crop.bottom);
                subElement->QueryFloatAttribute("top", &crop.top);
                proj.crop = crop;
            }
            else if (val == "Offset") {
                glm::vec3 offset = glm::vec3(0.f);
                subElement->QueryFloatAttribute("x", &offset[0]);
                subElement->QueryFloatAttribute("y", &offset[1]);
                subElement->QueryFloatAttribute("z", &offset[2]);
                proj.offset = offset;
            }
            if (val == "Background") {
                glm::vec4 color;
                subElement->QueryFloatAttribute("r", &color[0]);
                subElement->QueryFloatAttribute("g", &color[1]);
                subElement->QueryFloatAttribute("b", &color[2]);
                subElement->QueryFloatAttribute("a", &color[3]);
                proj.background = color;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::SphericalMirrorProjection parseSphericalMirrorProjection(tinyxml2::XMLElement* element) {
        sgct::config::SphericalMirrorProjection proj;
        if (element->Attribute("quality")) {
            proj.quality = sgct::core::cubeMapResolutionForQuality(element->Attribute("quality"));
        }

        float tilt;
        if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR) {
            proj.tilt = tilt;
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Background") {
                glm::vec4 color;
                subElement->QueryFloatAttribute("r", &color[0]);
                subElement->QueryFloatAttribute("g", &color[1]);
                subElement->QueryFloatAttribute("b", &color[2]);
                subElement->QueryFloatAttribute("a", &color[3]);
                proj.background = color;
            }
            else if (val == "Geometry") {
                if (subElement->Attribute("bottom")) {
                    proj.mesh.bottom = subElement->Attribute("bottom");
                }

                if (subElement->Attribute("left")) {
                    proj.mesh.left = subElement->Attribute("left");
                }

                if (subElement->Attribute("right")) {
                    proj.mesh.right = subElement->Attribute("right");
                }

                if (subElement->Attribute("top")) {
                    proj.mesh.top = subElement->Attribute("top");
                }
            }

            subElement = subElement->NextSiblingElement();
        }


        return proj;
    }

    [[nodiscard]] sgct::config::SpoutOutputProjection parseSpoutOutputProjection(tinyxml2::XMLElement* element) {
        sgct::config::SpoutOutputProjection proj;

        if (element->Attribute("quality")) {
            proj.quality = sgct::core::cubeMapResolutionForQuality(element->Attribute("quality"));
        }
        if (element->Attribute("mapping")) {
            std::string_view val = element->Attribute("mapping");
            if (val == "fisheye") {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Fisheye;
            }
            else if (val == "equirectangular") {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Equirectangular;
            }
            else if (val == "cubemap") {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Cubemap;
            }
            else {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Cubemap;
            }
        }
        if (element->Attribute("mappingSpoutName")) {
            proj.mappingSpoutName = element->Attribute("mappingSpoutName");
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Background") {
                glm::vec4 color;
                subElement->QueryFloatAttribute("r", &color[0]);
                subElement->QueryFloatAttribute("g", &color[1]);
                subElement->QueryFloatAttribute("b", &color[2]);
                subElement->QueryFloatAttribute("a", &color[3]);
                proj.background = color;
            }

            if (val == "Channels") {
                // @TODO(abock)  In the previous version it was ambiguous whether it should be
                //               initialized to false or true;  it did use 'true' in the end
                //               but I don't think that is the correct way though

                sgct::config::SpoutOutputProjection::Channels c;
                subElement->QueryBoolAttribute("Right", &c.right);
                subElement->QueryBoolAttribute("zLeft", &c.zLeft);
                subElement->QueryBoolAttribute("Bottom", &c.bottom);
                subElement->QueryBoolAttribute("Top", &c.top);
                subElement->QueryBoolAttribute("Left", &c.left);
                subElement->QueryBoolAttribute("zRight", &c.zRight);
                proj.channels = c;
            }

            if (val == "RigOrientation") {
                glm::vec3 orientation;
                subElement->QueryFloatAttribute("pitch", &orientation[0]);
                subElement->QueryFloatAttribute("yaw", &orientation[1]);
                subElement->QueryFloatAttribute("roll", &orientation[2]);
                proj.orientation = orientation;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::ProjectionPlane parseProjectionPlane(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;
        size_t i = 0;

        sgct::config::ProjectionPlane proj;

        tinyxml2::XMLElement* elem = element->FirstChildElement();
        while (elem) {
            std::string_view val = elem->Value();

            if (val == "Pos") {
                glm::vec3 pos;
                if (elem->QueryFloatAttribute("x", &pos[0]) == XML_NO_ERROR &&
                    elem->QueryFloatAttribute("y", &pos[1]) == XML_NO_ERROR &&
                    elem->QueryFloatAttribute("z", &pos[2]) == XML_NO_ERROR)
                {
                    switch (i % 3) {
                    case 0:
                        proj.lowerLeft = pos;
                        break;
                    case 1:
                        proj.upperLeft = pos;
                        break;
                    case 2:
                        proj.upperRight = pos;
                        break;
                    }

                    i++;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ProjectionPlane: Failed to parse coordinates from XML\n"
                    );
                }
            }

            elem = elem->NextSiblingElement();
        }

        return proj;

        //setCoordinateLowerLeft(*proj.lowerLeft);
        //initializedLowerLeftCorner = *proj.lowerLeft;
        //setCoordinateUpperLeft(*proj.upperLeft);
        //initializedUpperLeftCorner = *proj.upperLeft;
        //setCoordinateUpperRight(*proj.upperLeft);
        //initializedUpperRightCorner = *proj.upperLeft;
    }

    [[nodiscard]] sgct::config::MpcdiProjection parseMpcdi(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;

        sgct::config::MpcdiProjection proj;
        if (element->Attribute("id")) {
            proj.id = element->Attribute("id");
        }

        glm::vec2 vpPosition;
        if (element->QueryFloatAttribute("x", &vpPosition[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("y", &vpPosition[1]) == XML_NO_ERROR)
        {
            proj.position = vpPosition;
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Viewport: Failed to parse position from XML\n"
            );
        }

        glm::vec2 vpSize;
        if (element->QueryFloatAttribute("xSize", &vpSize[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("ySize", &vpSize[1]) == XML_NO_ERROR)
        {
            proj.size = vpSize;
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Viewport: Failed to parse size from XML\n"
            );
        }

        //glm::vec2 vpResolution;
        //if (element->QueryFloatAttribute("xResolution", &vpResolution[0]) == XML_NO_ERROR &&
        //    element->QueryFloatAttribute("yResolution", &vpResolution[1]) == XML_NO_ERROR)
        //{
        //    float expectedResolutionX = std::floor(vpResolution.x * winResX);
        //    float expectedResolutionY = std::floor(vpResolution.y * winResY);

        //    if (expectedResolutionX != vpResolution.x ||
        //        expectedResolutionY != vpResolution.y)
        //    {
        //        MessageHandler::instance()->print(
        //            MessageHandler::Level::Warning,
        //            "Viewport: MPCDI region expected resolution does not match window\n"
        //        );
        //    }

        //    // @TODO:  Do something with the resolution
        //}
        //else {
        //    MessageHandler::instance()->print(
        //        MessageHandler::Level::Error,
        //        "Viewport: Failed to parse resolution from XML\n"
        //    );
        //}

        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
        tinyxml2::XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "frustum") {
                bool hasRight = false;
                bool hasLeft = false;
                bool hasUp = false;
                bool hasDown = false;
                bool hasYaw = false;
                bool hasPitch = false;
                bool hasRoll = false;
                sgct::config::MpcdiProjection::Frustum frustum;
                tinyxml2::XMLElement* grandChild = child->FirstChildElement();
                while (grandChild) {
                    try {
                        std::string_view grandChildVal = grandChild->Value();
                        if (grandChildVal == "rightAngle") {
                            frustum.right = std::stof(grandChild->GetText());
                            hasRight = true;
                        }
                        else if (grandChildVal == "leftAngle") {
                            frustum.left = std::stof(grandChild->GetText());
                            hasLeft = true;
                        }
                        else if (grandChildVal == "upAngle") {
                            frustum.up = std::stof(grandChild->GetText());
                            hasUp = true;
                        }
                        else if (grandChildVal == "downAngle") {
                            frustum.down = std::stof(grandChild->GetText());
                            hasDown = true;
                        }
                        else if (grandChildVal == "yaw") {
                            yaw = std::stof(grandChild->GetText());
                            hasYaw = true;
                        }
                        else if (grandChildVal == "pitch") {
                            pitch = std::stof(grandChild->GetText());
                            hasPitch = true;
                        }
                        else if (grandChildVal == "roll") {
                            roll = std::stof(grandChild->GetText());
                            hasRoll = true;
                        }
                    }
                    catch (const std::invalid_argument&) {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "Viewport: Failed to parse frustum element from MPCDI XML\n"
                        );
                    }

                    grandChild = grandChild->NextSiblingElement();
                }


                const bool hasMissingField = !hasDown || !hasUp || !hasLeft || !hasRight ||
                    !hasYaw || !hasPitch || !hasRoll;

                if (hasMissingField) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse mpcdi projection FOV from XML\n"
                    );
                    return {};
                }

                proj.orientation = sgct::core::readconfig::parseMpcdiOrientationNode(yaw, pitch, roll);
            }
            child = child->NextSiblingElement();
        }

        return proj;
    }


} // namespace

namespace sgct::core {

Viewport::Viewport() : Viewport(0.f, 0.f, 0.f, 0.f) {}

Viewport::Viewport(float x, float y, float xSize, float ySize) {
    mPosition = glm::vec2(x, y);
    mSize = glm::vec2(xSize, ySize);
    mProjectionPlane.reset();
}

Viewport::~Viewport() {
    glDeleteTextures(1, &mOverlayTextureIndex);
    glDeleteTextures(1, &mBlendMaskTextureIndex);
    glDeleteTextures(1, &mBlackLevelMaskTextureIndex);
}

void Viewport::configure(tinyxml2::XMLElement* element) {
    if (element->Attribute("user")) {
        setUserName(element->Attribute("user"));
    }

    if (element->Attribute("name")) {
        setName(element->Attribute("name"));
    }

    if (element->Attribute("overlay")) {
        setOverlayTexture(element->Attribute("overlay"));
    }

    // for backward compability
    if (element->Attribute("mask")) {
        setBlendMaskTexture(element->Attribute("mask"));
    }

    if (element->Attribute("BlendMask")) {
        setBlendMaskTexture(element->Attribute("BlendMask"));
    }

    if (element->Attribute("BlackLevelMask")) {
        setBlackLevelMaskTexture(element->Attribute("BlackLevelMask"));
    }

    if (element->Attribute("mesh")) {
        setCorrectionMesh(element->Attribute("mesh"));
    }

    if (element->Attribute("hint")) {
        mMeshHint = element->Attribute("hint");
    }

    if (element->Attribute("tracked")) {
        std::string_view tracked = element->Attribute("tracked");
        setTracked(tracked == "true");
    }

    // get eye if set
    if (element->Attribute("eye")) {
        std::string_view eye = element->Attribute("eye");
        if (eye == "center") {
            setEye(Frustum::MonoEye);
        }
        else if (eye == "left") {
            setEye(Frustum::StereoLeftEye);
        }
        else if (eye == "right") {
            setEye(Frustum::StereoRightEye);
        }
    }

    tinyxml2::XMLElement* subElement = element->FirstChildElement();
    while (subElement) {
        using namespace tinyxml2;

        std::string_view val = subElement->Value();
        if (val == "Pos") {
            glm::vec2 position;
            if (subElement->QueryFloatAttribute("x", &position[0]) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("y", &position[1]) == XML_NO_ERROR)
            {
                setPos(std::move(position));
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Viewport: Failed to parse position from XML\n"
                );
            }
        }
        else if (val == "Size") {
            glm::vec2 size;
            if (subElement->QueryFloatAttribute("x", &size[0]) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("y", &size[1]) == XML_NO_ERROR)
            {
                setSize(std::move(size));
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Viewport: Failed to parse size from XML!\n"
                );
            }
        }
        else if (val == "PlanarProjection") {
            sgct::config::PlanarProjection proj = parsePlanarProjection(subElement);
            applyPlanarProjection(proj);
        }
        else if (val == "FisheyeProjection") {
            sgct::config::FisheyeProjection proj = parseFisheyeProjection(subElement);
            applyFisheyeProjection(proj);
        }
        else if (val == "SphericalMirrorProjection") {
            sgct::config::SphericalMirrorProjection proj = parseSphericalMirrorProjection(subElement);
            applySphericalMirrorProjection(proj);
        }
        else if (val == "SpoutOutputProjection") {
            sgct::config::SpoutOutputProjection proj = parseSpoutOutputProjection(subElement);
            applySpoutOutputProjection(proj);
        }
        else if (val == "Viewplane" || val == "Projectionplane") {
            sgct::config::ProjectionPlane proj = parseProjectionPlane(subElement);
            mProjectionPlane.setCoordinateLowerLeft(*proj.lowerLeft);
            mUnTransformedViewPlaneCoords.lowerLeft = *proj.lowerLeft;
            mProjectionPlane.setCoordinateUpperLeft(*proj.upperLeft);
            mUnTransformedViewPlaneCoords.upperLeft = *proj.upperLeft;
            mProjectionPlane.setCoordinateUpperRight(*proj.upperRight);
            mUnTransformedViewPlaneCoords.upperRight = *proj.upperRight;
        }

        // iterate
        subElement = subElement->NextSiblingElement();
    }
}


void Viewport::configureMpcdi(tinyxml2::XMLElement* element) {
    sgct::config::MpcdiProjection proj = parseMpcdi(element);

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
        mProjectionPlane.offset(*proj.offset);
    }
}

void Viewport::applyPlanarProjection(const sgct::config::PlanarProjection& proj) {
    if (proj.fov) {
        setViewPlaneCoordsUsingFOVs(
            proj.fov->up,
            -proj.fov->down,
            -proj.fov->left,
            proj.fov->right,
            proj.orientation ? *proj.orientation : glm::quat(),
            proj.fov->distance
        );
        mProjectionPlane.offset(*proj.offset);
    }
}

void Viewport::applyFisheyeProjection(const sgct::config::FisheyeProjection& proj) {
    std::unique_ptr<FisheyeProjection> fishProj = std::make_unique<FisheyeProjection>();
    fishProj->setUser(mUser);
    
    if (proj.fov) {
        fishProj->setFOV(*proj.fov);
    }
    if (proj.quality) {
        fishProj->setCubemapResolution(*proj.quality);
    }
    if (proj.method) {
        FisheyeProjection::FisheyeMethod m = [](config::FisheyeProjection::Method m) {
            switch (m) {
                default:
                case config::FisheyeProjection::Method::FourFace:
                    return FisheyeProjection::FisheyeMethod::FourFaceCube;
                case config::FisheyeProjection::Method::FiveFace:
                    return FisheyeProjection::FisheyeMethod::FiveFaceCube;
            }
        }(*proj.method);
        fishProj->setRenderingMethod(m);
    }
    if (proj.interpolation) {
        NonLinearProjection::InterpolationMode i =
            [](config::FisheyeProjection::Interpolation i) {
                switch (i) {
                    default:
                    case config::FisheyeProjection::Interpolation::Linear:
                        return NonLinearProjection::InterpolationMode::Linear;
                    case config::FisheyeProjection::Interpolation::Cubic:
                        return NonLinearProjection::InterpolationMode::Cubic;
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
    mNonLinearProjection = std::move(fishProj);
}

void Viewport::applySpoutOutputProjection(const sgct::config::SpoutOutputProjection& proj) {
#ifndef SGCT_HAS_SPOUT
    (void)proj;
    MessageHandler::instance()->print(
        MessageHandler::Level::Warning,
        "ReadConfig: Spout library not added to SGCT\n"
    );
    return;
#else
   
    std::unique_ptr<SpoutOutputProjection> spoutProj =
        std::make_unique<SpoutOutputProjection>();
    spoutProj->setUser(mUser);
    if (proj.quality) {
        spoutProj->setCubemapResolution(*proj.quality);
    }
    if (proj.mapping) {
        SpoutOutputProjection::Mapping m = [](sgct::config::SpoutOutputProjection::Mapping m) {
            switch (m) {
                default:
                case sgct::config::SpoutOutputProjection::Mapping::Fisheye:
                    return SpoutOutputProjection::Mapping::Fisheye;
                case sgct::config::SpoutOutputProjection::Mapping::Equirectangular:
                    return SpoutOutputProjection::Mapping::Equirectangular;
                case sgct::config::SpoutOutputProjection::Mapping::Cubemap:
                    return SpoutOutputProjection::Mapping::Cubemap;
            }
        }(*proj.mapping);
        spoutProj->setSpoutMapping(m);
    }
    if (proj.mappingSpoutName) {
        spoutProj->setSpoutMappingName(*proj.mappingSpoutName);
    }
    if (proj.background) {
        spoutProj->setClearColor(*proj.background);
    }
    if (proj.channels) {
        spoutProj->setSpoutChannels(&(*proj.channels));
    }
    if (proj.orientation) {
        spoutProj->setSpoutRigOrientation(*proj.orientation);
    }

    spoutProj->setUseDepthTransformation(true);
    mNonLinearProjection = std::move(spoutProj);
#endif
}

void Viewport::applySphericalMirrorProjection(const sgct::config::SphericalMirrorProjection& proj) {
    std::unique_ptr<SphericalMirrorProjection> sphericalMirrorProj =
        std::make_unique<SphericalMirrorProjection>();

    sphericalMirrorProj->setUser(mUser);
    sphericalMirrorProj->setCubemapResolution(*proj.quality);

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
    mNonLinearProjection = std::move(sphericalMirrorProj);
}

void Viewport::setOverlayTexture(std::string texturePath) {
    mOverlayFilename = std::move(texturePath);
}

void Viewport::setBlendMaskTexture(std::string texturePath) {
    mBlendMaskFilename = std::move(texturePath);
}

void Viewport::setBlackLevelMaskTexture(std::string texturePath) {
    mBlackLevelMaskFilename = std::move(texturePath);
}

void Viewport::setCorrectionMesh(std::string meshPath) {
    mMeshFilename = std::move(meshPath);
}

void Viewport::setMpcdiWarpMesh(std::vector<unsigned char> data) {
    mMpcdiWarpMesh = std::move(data);
}

void Viewport::setTracked(bool state) {
    mTracked = state;
}

void Viewport::loadData() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Viewport: loading GPU data for '%s'\n", mName.c_str()
    );
        
    if (!mOverlayFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            mOverlayTextureIndex,
            mOverlayFilename,
            true,
            1
        );
    }

    if (!mBlendMaskFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            mBlendMaskTextureIndex,
            mBlendMaskFilename,
            true,
            1
        );
    }

    if (!mBlackLevelMaskFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            mBlackLevelMaskTextureIndex,
            mBlackLevelMaskFilename,
            true,
            1
        );
    }

    if (!mMpcdiWarpMesh.empty()) {
        mCorrectionMesh = mCM.readAndGenerateMesh(
            "mesh.mpcdi",
            *this,
            CorrectionMesh::parseHint("mpcdi")
        );
    }
    else {
        // load default if mMeshFilename is empty
        mCorrectionMesh = mCM.readAndGenerateMesh(
            mMeshFilename,
            *this,
            CorrectionMesh::parseHint(mMeshHint)
        );
    }
}

void Viewport::renderQuadMesh() const {
    if (mEnabled) {
        mCM.renderQuadMesh();
    }
}

void Viewport::renderWarpMesh() const {
    if (mEnabled) {
        mCM.renderWarpMesh();
    }
}

void Viewport::renderMaskMesh() const {
    if (mEnabled) {
        mCM.renderMaskMesh();
    }
}

bool Viewport::hasOverlayTexture() const {
    return mOverlayTextureIndex != 0;
}

bool Viewport::hasBlendMaskTexture() const {
    return mBlendMaskTextureIndex != 0;
}

bool Viewport::hasBlackLevelMaskTexture() const {
    return mBlackLevelMaskTextureIndex != 0;
}

bool Viewport::hasSubViewports() const {
    return mNonLinearProjection != nullptr;
}

bool Viewport::hasCorrectionMesh() const {
    return mCorrectionMesh;
}

bool Viewport::isTracked() const {
    return mTracked;
}

unsigned int Viewport::getOverlayTextureIndex() const {
    return mOverlayTextureIndex;
}

unsigned int Viewport::getBlendMaskTextureIndex() const {
    return mBlendMaskTextureIndex;
}

unsigned int Viewport::getBlackLevelMaskTextureIndex() const {
    return mBlackLevelMaskTextureIndex;
}

CorrectionMesh& Viewport::getCorrectionMeshPtr() {
    return mCM;
}

NonLinearProjection* Viewport::getNonLinearProjection() const {
    return mNonLinearProjection.get();
}

const std::vector<unsigned char>& Viewport::mpcdiWarpMesh() const {
    return mMpcdiWarpMesh;
}

} // namespace sgct::core
