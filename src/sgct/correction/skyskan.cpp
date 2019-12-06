/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/skyskan.h>

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/viewport.h>
#include <sgct/user.h>
#include <optional>

#define Error(code, msg) sgct::Error(sgct::Error::Component::SkySkan, code, msg)

namespace sgct::correction {

Buffer generateSkySkanMesh(const std::string& path, BaseViewport& parent) {
    Buffer buf;

    Log::Info("Reading SkySkan mesh data from '%s'", path.c_str());

    FILE* meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        throw Error(2080, "Failed to open file " + path);
    }

    std::optional<float> azimuth;
    std::optional<float> elevation;
    std::optional<float> hFov;
    std::optional<float> vFov;
    glm::vec2 fovTweaks(1.f);
    glm::vec2 uvTweaks(1.f);
    bool areDimsSet = false;

    unsigned int sizeX = 0;
    unsigned int sizeY = 0;
    unsigned int counter = 0;

    while (!feof(meshFile)) {
        constexpr const int MaxLineLength = 1024;
        char lineBuffer[MaxLineLength];
        char* res = fgets(lineBuffer, MaxLineLength, meshFile);
        if (res == nullptr) {
            continue;
        }

        float x, y, u, v;
        if (sscanf(lineBuffer, "Dome Azimuth=%f", &v) == 1) {
            azimuth = v;
        }
        else if (sscanf(lineBuffer, "Dome Elevation=%f", &v) == 1) {
            elevation = v;
        }
        else if (sscanf(lineBuffer, "Horizontal FOV=%f", &v) == 1) {
            hFov = v;
        }
        else if (sscanf(lineBuffer, "Vertical FOV=%f", &v) == 1) {
            vFov = v;
        }
        else if (sscanf(lineBuffer, "Horizontal Tweak=%f", &fovTweaks[0]) == 1) {}
        else if (sscanf(lineBuffer, "Vertical Tweak=%f", &fovTweaks[1]) == 1) {}
        else if (sscanf(lineBuffer, "U Tweak=%f", &uvTweaks[0]) == 1) {}
        else if (sscanf(lineBuffer, "V Tweak=%f", &uvTweaks[1]) == 1) {}
        else if (!areDimsSet && sscanf(lineBuffer, "%u %u", &sizeX, &sizeY) == 2) {
            areDimsSet = true;
            buf.vertices.resize(sizeX * sizeY);
        }
        else if (areDimsSet && sscanf(lineBuffer, "%f %f %f %f", &x, &y, &u, &v) == 4) {
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

    if (!areDimsSet || !azimuth.has_value() || !elevation.has_value() ||
        !hFov.has_value() || *hFov <= 0.f)
    {
        throw Error(2081, "Data reading error");
    }

    // create frustums and projection matrices
    if (!vFov.has_value() || *vFov <= 0.f) {
        // half the width (radius is one unit, cancels it self out)
        const float hw = tan(glm::radians<float>(*hFov) / 2.f);
        // half height
        const float hh = (1200.f / 2048.f) * hw;
        vFov = 2.f * glm::degrees<float>(atan(hh));

        Log::Info("HFOV: %f VFOV: %f", *hFov, *vFov);
    }

    if (fovTweaks[0] > 0.f) {
        hFov = *hFov * fovTweaks[0];
    }
    if (fovTweaks[1] > 0.f) {
        vFov = *vFov * fovTweaks[1];
    }

    glm::quat rotQuat = glm::quat(1.f, 0.f, 0.f, 0.f);
    rotQuat = glm::rotate(rotQuat, glm::radians(-*azimuth), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(*elevation), glm::vec3(1.f, 0.f, 0.f));

    parent.user().setPos(glm::vec3(0.f));
    const float vHalf = *vFov / 2.f;
    const float hHalf = *hFov / 2.f;
    parent.setViewPlaneCoordsUsingFOVs(vHalf, -vHalf, -hHalf, hHalf, rotQuat);
    Engine::instance().updateFrustums();

    for (unsigned int c = 0; c < (sizeX - 1); c++) {
        for (unsigned int r = 0; r < (sizeY - 1); r++) {
            const unsigned int i0 = r * sizeX + c;
            const unsigned int i1 = r * sizeX + (c + 1);
            const unsigned int i2 = (r + 1) * sizeX + (c + 1);
            const unsigned int i3 = (r + 1) * sizeX + c;

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
        const glm::vec2& s = parent.size();
        const glm::vec2& p = parent.position();

        // convert to [-1, 1]
        vertex.x = 2.f * (vertex.x * s.x + p.x) - 1.f;
        vertex.y = 2.f * ((1.f - vertex.y) * s.y + p.y) - 1.f;

        vertex.s = vertex.s * s.x + p.x;
        vertex.t = vertex.t * s.y + p.y;
    }

    buf.geometryType = GL_TRIANGLES;
    return buf;
}

} // namespace sgct::correction
