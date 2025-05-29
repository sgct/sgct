/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/skyskan.h>

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <scn/scan.h>
#include <fstream>
#include <optional>

#define Error(code, msg) sgct::Error(sgct::Error::Component::SkySkan, code, msg)

namespace sgct::correction {

Buffer generateSkySkanMesh(const std::filesystem::path& path, BaseViewport& parent) {
    ZoneScoped;

    Buffer buf;

    Log::Info(std::format("Reading SkySkan mesh data from '{}'", path.string()));

    std::ifstream meshFile = std::ifstream(path);
    if (!meshFile.good()) {
        throw Error(2090, std::format("Failed to open file '{}'", path.string()));
    }

    std::optional<float> azimuth;
    std::optional<float> elevation;
    std::optional<float> hFov;
    std::optional<float> vFov;
    vec2 fovTweaks{ 1.f, 1.f };
    vec2 uvTweaks{ 1.f, 1.f };
    bool areDimsSet = false;

    unsigned int sizeX = 0;
    unsigned int sizeY = 0;
    unsigned int counter = 0;

    std::string line;
    while (std::getline(meshFile, line)) {
        if (auto r = scn::scan<float>(line, "Dome Azimuth={}");  r) {
            azimuth = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "Dome Elevation={}");  r) {
            elevation = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "Horizontal FOV={}");  r) {
            hFov = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "Vertical FOV={}");  r) {
            vFov = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "Horizontal Tweak={}");  r) {
            fovTweaks.x = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "Vertical Tweak={}");  r) {
            fovTweaks.y = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "U Tweak={}");  r) {
            uvTweaks.x = r->value();
            break;
        }
        if (auto r = scn::scan<float>(line, "V Tweak={}");  r) {
            uvTweaks.y = r->value();
            break;
        }
        if (auto r = scn::scan<unsigned int, unsigned int>(line, "{} {}");
            r && !areDimsSet)
        {
            areDimsSet = true;
            std::tie(sizeX, sizeY) = r->values();
            buf.vertices.resize(static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY));
            break;
        }
        if (auto r = scn::scan<float, float, float, float>(line, "{} {} {} {}");
            r && areDimsSet)
        {
            auto& [x, y, u, v] = r->values();
            if (uvTweaks.x > -1.f) {
                u *= uvTweaks.x;
            }

            if (uvTweaks.y > -1.f) {
                v *= uvTweaks.y;
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
            break;
        }
    }

    if (!areDimsSet || !azimuth.has_value() || !elevation.has_value() ||
        !hFov.has_value() || *hFov <= 0.f)
    {
        throw Error(2091, std::format("Data reading error in file '{}'", path.string()));
    }

    // create frustums and projection matrices
    if (!vFov.has_value() || *vFov <= 0.f) {
        // half the width (radius is one unit, cancels it self out)
        const float hw = std::tan(glm::radians<float>(*hFov) / 2.f);
        // half height
        const float hh = (1200.f / 2048.f) * hw;
        vFov = 2.f * glm::degrees<float>(std::atan(hh));

        Log::Info(std::format("HFOV: {} VFOV: {}", *hFov, *vFov));
    }

    if (fovTweaks.x > 0.f) {
        hFov = *hFov * fovTweaks.x;
    }
    if (fovTweaks.y > 0.f) {
        vFov = *vFov * fovTweaks.y;
    }

    glm::quat rotQuat = glm::quat(1.f, 0.f, 0.f, 0.f);
    rotQuat = glm::rotate(rotQuat, glm::radians(-*azimuth), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(*elevation), glm::vec3(1.f, 0.f, 0.f));

    parent.user().setPos(vec3{ 0.f, 0.f, 0.f });
    const float vHalf = *vFov / 2.f;
    const float hHalf = *hFov / 2.f;
    parent.setViewPlaneCoordsUsingFOVs(
        vHalf,
        -vHalf,
        -hHalf,
        hHalf,
        quat(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w)
    );
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

    for (Buffer::Vertex& vertex : buf.vertices) {
        const vec2& s = parent.size();
        const vec2& p = parent.position();

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
