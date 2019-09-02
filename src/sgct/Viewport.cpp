/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/Viewport.h>

#include <sgct/ClusterManager.h>
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
} // namespace

namespace sgct_core {

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
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
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
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Viewport: Failed to parse size from XML!\n"
                );
            }
        }
        else if (val == "PlanarProjection") {
            parsePlanarProjection(subElement);
        }
        else if (val == "FisheyeProjection") {
            parseFisheyeProjection(subElement);
        }
        else if (val == "SphericalMirrorProjection") {
            parseSphericalMirrorProjection(subElement);
        }
        else if (val == "SpoutOutputProjection") {
            parseSpoutOutputProjection(subElement);
        }
        else if (val == "Viewplane" || val == "Projectionplane") {
            mProjectionPlane.configure(
                subElement,
                mUnTransformedViewPlaneCoords.lowerLeft,
                mUnTransformedViewPlaneCoords.upperLeft,
                mUnTransformedViewPlaneCoords.upperRight
            );
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }
}

// @TODO(abock)  The function signature needs to be cleaned up during a pass of SGCTMpcdi
//               file.  It's not clear what is happening with the double pointers here and
//               which assignments are actually used
void Viewport::configureMpcdi(tinyxml2::XMLElement* element, int winResX, int winResY) {
    using namespace tinyxml2;

    if (element->Attribute("id")) {
        setName(element->Attribute("id"));
    }

    glm::vec2 vpPosition;
    if (element->QueryFloatAttribute("x", &vpPosition[0]) == XML_NO_ERROR &&
        element->QueryFloatAttribute("y", &vpPosition[1]) == XML_NO_ERROR)
    {
        setPos(std::move(vpPosition));
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
        setSize(std::move(vpSize));
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Viewport: Failed to parse size from XML\n"
        );
    }

    glm::vec2 vpResolution;
    if (element->QueryFloatAttribute("xResolution", &vpResolution[0]) == XML_NO_ERROR &&
        element->QueryFloatAttribute("yResolution", &vpResolution[1]) == XML_NO_ERROR)
    {
        float expectedResolutionX = std::floor(vpResolution.x * winResX);
        float expectedResolutionY = std::floor(vpResolution.y * winResY);
        
        if (expectedResolutionX != vpResolution.x ||
            expectedResolutionY != vpResolution.y)
        {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Warning,
                "Viewport: MPCDI region expected resolution does not match window\n"
            );
        }

        // @TODO:  Do something with the resolution
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Viewport: Failed to parse resolution from XML\n"
        );
    }
    
    std::optional<float> down;
    std::optional<float> up;
    std::optional<float> left;
    std::optional<float> right;
    std::optional<float> yaw;
    std::optional<float> pitch;
    std::optional<float> roll;


    tinyxml2::XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view val = child->Value();
        if (val == "frustum") {
            float distance = 10.f;
            glm::quat rotQuat;
            glm::vec3 offset(0.f, 0.f, 0.f);

            tinyxml2::XMLElement* grandChild = child->FirstChildElement();
            while (grandChild) {
                bool frustumTagFound = false;

                if (!frustumTagFound) {
                    right = parseFrustumElement(*grandChild, "rightAngle");
                    frustumTagFound = right.has_value();
                }
                if (!frustumTagFound) {
                    left = parseFrustumElement(*grandChild, "leftAngle");
                    frustumTagFound = left.has_value();
                }
                if (!frustumTagFound) {
                    up = parseFrustumElement(*grandChild, "upAngle");
                    frustumTagFound = up.has_value();
                }
                if (!frustumTagFound) {
                    down = parseFrustumElement(*grandChild, "downAngle");
                    frustumTagFound = down.has_value();
                }
                if (!frustumTagFound) {
                    yaw = parseFrustumElement(*grandChild, "yaw");
                    frustumTagFound = yaw.has_value();
                }
                if (!frustumTagFound) {
                    pitch = parseFrustumElement(*grandChild, "pitch");
                    frustumTagFound = pitch.has_value();
                }
                if (!frustumTagFound) {
                    roll = parseFrustumElement(*grandChild, "roll");
                    frustumTagFound = roll.has_value();
                }

                grandChild = grandChild->NextSiblingElement();
            }

            bool hasMissingField = !down.has_value() || !up.has_value() ||
                !left.has_value() || !right.has_value() || !yaw.has_value() ||
                !pitch.has_value() || !roll.has_value();

            if (hasMissingField) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Viewport: Failed to parse mpcdi projection FOV from XML\n"
                );
                return;
            }

            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "Viewport: Adding mpcdi FOV d=%f l=%f r=%f u=%f y=%f p=%f r=%f\n",
                *down, *left, *right, *up, *yaw, *pitch, *roll
            );
            rotQuat = readconfig::parseMpcdiOrientationNode(*yaw, *pitch, *roll);
            setViewPlaneCoordsUsingFOVs(*up, *down, *left, *right, rotQuat, distance);
            mProjectionPlane.offset(offset);
        }
        child = child->NextSiblingElement();
    }
}

void Viewport::parsePlanarProjection(tinyxml2::XMLElement* element) {
    using namespace tinyxml2;

    bool validFOV = false;
    float down;
    float left;
    float right;
    float up;
    float distance = 10.f;
    glm::quat rotQuat;
    glm::vec3 offset(0.f, 0.f, 0.f);

    XMLElement* subElement = element->FirstChildElement();
    while (subElement) {
        std::string_view val = subElement->Value();

        if (val == "FOV") {
            if (subElement->QueryFloatAttribute("down", &down) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("left", &left) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("right", &right) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("up", &up) == XML_NO_ERROR)
            {
                validFOV = true;

                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "Viewport: Adding planar projection left=%f right=%f up=%f down=%f\n",
                    left, right, up, down
                );

                float tanLeft = tan(glm::radians(left));
                float tanRight = tan(glm::radians(right));
                float tanBottom = tan(glm::radians(down));
                float tanTop = tan(glm::radians(up));

                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "Tan angles: tanLeft=%f tanRight=%f tanBottom=%f tanTop=%f\n "
                    "width=%f\n height=%f\n",
                    tanLeft, tanRight, tanBottom, tanTop, tanRight + tanLeft,
                    tanTop + tanBottom
                );
            }
            else {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Viewport: Failed to parse planar projection FOV from XML\n"
                );
            }

            subElement->QueryFloatAttribute("distance", &distance);
        }
        else if (val == "Orientation") {
            rotQuat = readconfig::parseOrientationNode(subElement);
        }
        else if (val == "Offset") {
            subElement->QueryFloatAttribute("x", &offset[0]);
            subElement->QueryFloatAttribute("y", &offset[1]);
            subElement->QueryFloatAttribute("z", &offset[2]);
        }

        subElement = subElement->NextSiblingElement();
    }

    if (validFOV) {
        setViewPlaneCoordsUsingFOVs(up, -down, -left, right, rotQuat, distance);
        mProjectionPlane.offset(offset);
    }
}

void Viewport::parseFisheyeProjection(tinyxml2::XMLElement* element) {
    std::unique_ptr<FisheyeProjection> fishProj = std::make_unique<FisheyeProjection>();
    fishProj->setUser(mUser);
    
    float fov;
    if (element->QueryFloatAttribute("fov", &fov) == tinyxml2::XML_NO_ERROR) {
        fishProj->setFOV(fov);
    }

    if (element->Attribute("quality")) {
        const int res = cubeMapResolutionForQuality(element->Attribute("quality"));
        if (res > 0) {
            fishProj->setCubemapResolution(res);
        }
    }

    if (element->Attribute("method")) {
        std::string_view method = element->Attribute("method");
        fishProj->setRenderingMethod(
            method == "five_face_cube" ?
                FisheyeProjection::FisheyeMethod::FiveFaceCube :
                FisheyeProjection::FisheyeMethod::FourFaceCube
        );
    }

    if (element->Attribute("interpolation")) {
        std::string_view interpolation = element->Attribute("interpolation");
        fishProj->setInterpolationMode(
            interpolation == "cubic" ?
                NonLinearProjection::InterpolationMode::Cubic :
                NonLinearProjection::InterpolationMode::Linear
        );
    }

    float tilt;
    if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR) {
        fishProj->setTilt(tilt);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ReadConfig: Setting fisheye tilt to %f degrees\n", tilt
        );
    }

    float diameter;
    if (element->QueryFloatAttribute("diameter", &diameter) == tinyxml2::XML_NO_ERROR) {
        fishProj->setDomeDiameter(diameter);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ReadConfig: Setting fisheye diameter to %f meters\n", diameter
        );
    }


    tinyxml2::XMLElement* subElement = element->FirstChildElement();
    while (subElement) {
        std::string_view val = subElement->Value();

        if (val == "Crop") {
            float cropLeft = 0.f;
            float cropRight = 0.f;
            float cropBottom = 0.f;
            float cropTop = 0.f;

            subElement->QueryFloatAttribute("left", &cropLeft);
            subElement->QueryFloatAttribute("right", &cropRight);
            subElement->QueryFloatAttribute("bottom", &cropBottom);
            subElement->QueryFloatAttribute("top", &cropTop);

            fishProj->setCropFactors(cropLeft, cropRight, cropBottom, cropTop);
        }
        else if (val == "Offset") {
            glm::vec3 offset = glm::vec3(0.f);
            subElement->QueryFloatAttribute("x", &offset[0]);
            subElement->QueryFloatAttribute("y", &offset[1]);
            subElement->QueryFloatAttribute("z", &offset[2]);
            fishProj->setBaseOffset(offset);
        }
        if (val == "Background") {
            glm::vec4 color;
            subElement->QueryFloatAttribute("r", &color[0]);
            subElement->QueryFloatAttribute("g", &color[1]);
            subElement->QueryFloatAttribute("b", &color[2]);
            subElement->QueryFloatAttribute("a", &color[3]);
            fishProj->setClearColor(color);
        }

        subElement = subElement->NextSiblingElement();
    }

    fishProj->setUseDepthTransformation(true);
    mNonLinearProjection = std::move(fishProj);
}

void Viewport::parseSpoutOutputProjection(tinyxml2::XMLElement* element) {
#ifndef SGCT_HAS_SPOUT
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Warning,
        "ReadConfig: Spout library not added to SGCT\n"
    );
    return;
#endif
    std::unique_ptr<SpoutOutputProjection> spoutProj =
        std::make_unique<SpoutOutputProjection>();

    spoutProj->setUser(mUser);

    if (element->Attribute("quality")) {
        const int res = cubeMapResolutionForQuality(element->Attribute("quality"));
        if (res > 0) {
            spoutProj->setCubemapResolution(res);
        }
    }
    if (element->Attribute("mapping")) {
        std::string_view val = element->Attribute("mapping");
        if (val == "fisheye") {
            spoutProj->setSpoutMapping(SpoutOutputProjection::Mapping::Fisheye);
        }
        else if (val == "equirectangular") {
            spoutProj->setSpoutMapping(SpoutOutputProjection::Mapping::Equirectangular);
        }
        else if (val == "cubemap") {
            spoutProj->setSpoutMapping(SpoutOutputProjection::Mapping::Cubemap);
        }
        else {
            spoutProj->setSpoutMapping(SpoutOutputProjection::Mapping::Cubemap);
        }
    }
    if (element->Attribute("mappingSpoutName")) {
        spoutProj->setSpoutMappingName(element->Attribute("mappingSpoutName"));
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
            spoutProj->setClearColor(color);
        }

        if (val == "Channels") {
            // @TODO(abock)  In the previous version it was ambiguous whether it should be
            //               initialized to false or true;  it did use 'true' in the end
            //               but I don't think that is the correct way though

            std::array<bool, SpoutOutputProjection::NFaces> channel = { true };
            for (size_t i = 0; i < SpoutOutputProjection::NFaces; i++) {
                subElement->QueryBoolAttribute(
                    SpoutOutputProjection::CubeMapFaceName[i],
                    &channel[i]
                );
            }

            spoutProj->setSpoutChannels(channel.data());
        }

        if (val == "RigOrientation") {
            glm::vec3 orientation;
            subElement->QueryFloatAttribute("pitch", &orientation[0]);
            subElement->QueryFloatAttribute("yaw", &orientation[1]);
            subElement->QueryFloatAttribute("roll", &orientation[2]);
            spoutProj->setSpoutRigOrientation(orientation);
        }

        subElement = subElement->NextSiblingElement();
    }

    spoutProj->setUseDepthTransformation(true);
    mNonLinearProjection = std::move(spoutProj);
}

void Viewport::parseSphericalMirrorProjection(tinyxml2::XMLElement* element) {
    std::unique_ptr<SphericalMirrorProjection> sphericalMirrorProj =
        std::make_unique<SphericalMirrorProjection>();

    sphericalMirrorProj->setUser(mUser);

    if (element->Attribute("quality")) {
        const int res = cubeMapResolutionForQuality(element->Attribute("quality"));
        if (res > 0) {
            sphericalMirrorProj->setCubemapResolution(res);
        }
    }

    float tilt;
    if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR) {
        sphericalMirrorProj->setTilt(tilt);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ReadConfig: Setting spherical mirror tilt to %f degrees\n", tilt
        );
    }

    tinyxml2::XMLElement* subElement = element->FirstChildElement();
    struct {
        std::string bottom;
        std::string left;
        std::string right;
        std::string top;
    } meshes;

    while (subElement) {
        std::string_view val = subElement->Value();

        if (val == "Background") {
            glm::vec4 color;
            subElement->QueryFloatAttribute("r", &color[0]);
            subElement->QueryFloatAttribute("g", &color[1]);
            subElement->QueryFloatAttribute("b", &color[2]);
            subElement->QueryFloatAttribute("a", &color[3]);
            sphericalMirrorProj->setClearColor(color);
        }
        else if (val == "Geometry") {
            if (subElement->Attribute("bottom")) {
                meshes.bottom = subElement->Attribute("bottom");
            }

            if (subElement->Attribute("left")) {
                meshes.left = subElement->Attribute("left");
            }

            if (subElement->Attribute("right")) {
                meshes.right = subElement->Attribute("right");
            }

            if (subElement->Attribute("top")) {
                meshes.top = subElement->Attribute("top");
            }
        }

        subElement = subElement->NextSiblingElement();
    }

    sphericalMirrorProj->setMeshPaths(
        std::move(meshes.bottom),
        std::move(meshes.left),
        std::move(meshes.right),
        std::move(meshes.top)
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
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Viewport: loading GPU data for '%s'\n", mName.c_str()
    );
        
    if (!mOverlayFilename.empty()) {
        sgct::TextureManager::instance()->loadUnManagedTexture(
            mOverlayTextureIndex,
            mOverlayFilename,
            true,
            1
        );
    }

    if (!mBlendMaskFilename.empty()) {
        sgct::TextureManager::instance()->loadUnManagedTexture(
            mBlendMaskTextureIndex,
            mBlendMaskFilename,
            true,
            1
        );
    }

    if (!mBlackLevelMaskFilename.empty()) {
        sgct::TextureManager::instance()->loadUnManagedTexture(
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

} // namespace sgct_core
