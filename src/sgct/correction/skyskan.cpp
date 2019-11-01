/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/skyskan.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/viewport.h>
#include <sgct/user.h>

#if (_MSC_VER >= 1400)
    #define _sscanf sscanf_s
#else
    #define _sscanf sscanf
#endif

namespace {
    constexpr const int MaxLineLength = 1024;
} // namespace

namespace sgct::core::correction {

Buffer generateSkySkanMesh(const std::string& path, core::Viewport& parent) {
    Buffer buf;

    MessageHandler::printInfo(
        "CorrectionMesh: Reading SkySkan mesh data from '%s'", path.c_str()
    );

    FILE* meshFile = nullptr;
    bool loadSuccess = false;
#if (_MSC_VER >= 1400)
    loadSuccess = fopen_s(&meshFile, path.c_str(), "r") == 0;
#else
    meshFile = fopen(path.c_str(), "r");
    loadSuccess = meshFile != nullptr;
#endif
    if (!loadSuccess) {
        char ErrorBuffer[1024];
        sprintf(ErrorBuffer, "Failed to open warping mesh file '%s'", path.c_str());
        throw std::runtime_error(ErrorBuffer);
    }

    float azimuth = 0.f;
    float elevation = 0.f;
    float hFov = 0.f;
    float vFov = 0.f;
    glm::vec2 fovTweaks(1.f);
    glm::vec2 uvTweaks(1.f);
    bool dimensionsSet = false;
    bool azimuthSet = false;
    bool elevationSet = false;
    bool hFovSet = false;
    bool vFovSet = false;
    float x, y, u, v;

    unsigned int sizeX = 0;
    unsigned int sizeY = 0;
    unsigned int counter = 0;

    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        char* res = fgets(lineBuffer, MaxLineLength, meshFile);
        if (res == nullptr) {
            continue;
        }

        if (_sscanf(lineBuffer, "Dome Azimuth=%f", &azimuth) == 1) {
            azimuthSet = true;
        }
        else if (_sscanf(lineBuffer, "Dome Elevation=%f", &elevation) == 1) {
            elevationSet = true;
        }
        else if (_sscanf(lineBuffer, "Horizontal FOV=%f", &hFov) == 1) {
            hFovSet = true;
        }
        else if (_sscanf(lineBuffer, "Vertical FOV=%f", &vFov) == 1) {
            vFovSet = true;
        }
        else if (_sscanf(lineBuffer, "Horizontal Tweak=%f", &fovTweaks[0]) == 1) {
            ;
        }
        else if (_sscanf(lineBuffer, "Vertical Tweak=%f", &fovTweaks[1]) == 1) {
            ;
        }
        else if (_sscanf(lineBuffer, "U Tweak=%f", &uvTweaks[0]) == 1) {
            ;
        }
        else if (_sscanf(lineBuffer, "V Tweak=%f", &uvTweaks[1]) == 1) {
            ;
        }
        else if (!dimensionsSet &&
                 _sscanf(lineBuffer, "%u %u", &sizeX, &sizeY) == 2)
        {
            dimensionsSet = true;
            buf.vertices.resize(sizeX * sizeY);
        }
        else if (dimensionsSet && _sscanf(lineBuffer, "%f %f %f %f", &x, &y, &u, &v) == 4)
        {
            if (uvTweaks[0] > -1.f) {
                u *= uvTweaks[0];
            }

            if (uvTweaks[1] > -1.f) {
                v *= uvTweaks[1];
            }
            
            buf.vertices[counter].x = x;
            buf.vertices[counter].y = y;
            buf.vertices[counter].s = u;
            buf.vertices[counter].t = 1.f - v;

            buf.vertices[counter].r = 1.f;
            buf.vertices[counter].g = 1.f;
            buf.vertices[counter].b = 1.f;
            buf.vertices[counter].a = 1.f;
            counter++;
        }
    }

    fclose(meshFile);

    if (!dimensionsSet || !azimuthSet || !elevationSet || !hFovSet || hFov <= 0.f) {
        throw std::runtime_error("Data reading error");
    }

    // create frustums and projection matrices
    if (!vFovSet || vFov <= 0.f) {
        // half the width (radius is one unit, cancels it self out)
        const float hw = tan(glm::radians<float>(hFov) / 2.f);
        // half height
        const float hh = (1200.f / 2048.f) * hw;
        vFov = 2.f * glm::degrees<float>(atan(hh));

        MessageHandler::printInfo("HFOV: %f VFOV: %f", hFov, vFov);
    }

    if (fovTweaks[0] > 0.f) {
        hFov *= fovTweaks[0];
    }
    if (fovTweaks[1] > 0.f) {
        vFov *= fovTweaks[1];
    }

    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(-azimuth), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(elevation), glm::vec3(1.f, 0.f, 0.f));

    parent.getUser().setPos(glm::vec3(0.f));
    parent.setViewPlaneCoordsUsingFOVs(
        vFov / 2.f,
        -vFov / 2.f,
        -hFov / 2.f,
        hFov / 2.f,
        rotQuat
    );

    Engine::instance().updateFrustums();

    for (unsigned int c = 0; c < (sizeX - 1); c++) {
        for (unsigned int r = 0; r < (sizeY - 1); r++) {
            const unsigned int i0 = r * sizeX + c;
            const unsigned int i1 = r * sizeX + (c + 1);
            const unsigned int i2 = (r + 1) * sizeX + (c + 1);
            const unsigned int i3 = (r + 1) * sizeX + c;

            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            // triangle 1
            if (buf.vertices[i0].x != -1.f && buf.vertices[i0].y != -1.f &&
                buf.vertices[i1].x != -1.f && buf.vertices[i1].y != -1.f &&
                buf.vertices[i2].x != -1.f && buf.vertices[i2].y != -1.f)
            {
                buf.indices.push_back(i0);
                buf.indices.push_back(i1);
                buf.indices.push_back(i2);
            }

            // triangle 2
            if (buf.vertices[i0].x != -1.f && buf.vertices[i0].y != -1.f &&
                buf.vertices[i2].x != -1.f && buf.vertices[i2].y != -1.f &&
                buf.vertices[i3].x != -1.f && buf.vertices[i3].y != -1.f)
            {
                buf.indices.push_back(i0);
                buf.indices.push_back(i2);
                buf.indices.push_back(i3);
            }
        }
    }

    for (CorrectionMeshVertex& vertex : buf.vertices) {
        const glm::vec2& s = parent.getSize();
        const glm::vec2& p = parent.getPosition();

        // convert to [-1, 1]
        vertex.x = 2.f * (vertex.x * s.x + p.x) - 1.f;
        vertex.y = 2.f * ((1.f - vertex.y) * s.y + p.y) - 1.f;

        vertex.s = vertex.s * s.x + p.x;
        vertex.t = vertex.t * s.y + p.y;
    }

    buf.geometryType = GL_TRIANGLES;

    return buf;
}

} // namespace sgct::core::correction
