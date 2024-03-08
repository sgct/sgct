/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/scalable.h>

#include <sgct/baseviewport.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>

namespace {
    struct Data {
        int nVertices = 0;
        int nFaces = 0;

        struct Vertex {
            float x = 0.f;
            float y = 0.f;
            int intensity = 255;
            float s = 0.f;
            float t = 0.f;
        };
        std::vector<Vertex> vertices;

        struct Face {
            unsigned int f1 = 0;
            unsigned int f2 = 0;
            unsigned int f3 = 0;
        };
        std::vector<Face> faces;

        struct {
            float left = 0.f;
            float right = 1.f;
            float top = 1.f;
            float bottom = 0.f;
        } ortho;

        struct {
            struct {
                float x = 0.f;
                float y = 0.f;
                float z = 0.f;
            } offset;
            bool hasOffset = false;

            struct {
                float pitch = 0.f;
                float yaw = 0.f;
                float roll = 0.f;
            } direction;

            struct {
                float left = 0.f;
                float right = 0.f;
                float top = 0.f;
                float bottom = 0.f;
            } fov;
            bool hasFov = false;
        } perspective;

        struct {
            bool useAngles = false;
            float yaw = 0.f;
            float pitch = 0.f;
            float roll = 0.f;
        } frustumEulerAngles;

        struct {
            int x = 0;
            int y = 0;
        } resolution;

        float gamma = 2.2f;
        bool doNotWarp = false;
        std::string label;

        bool applyMask = false;
        bool applyBlackLevel = false;
        bool applyColor = false;
    };
} // namespace

namespace sgct::correction {

Buffer generateScalableMesh(const std::filesystem::path& path, BaseViewport& parent) {
    ZoneScoped;

    Log::Info(fmt::format("Reading scalable mesh data from '{}'", path));

    std::ifstream file(path);
    if (!file.good()) {
        throw Error(
            Error::Component::Scalable, 2060, fmt::format("Failed to open '{}'", path)
        );
    }

    Data data;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::string_view v = line;
        size_t separator = v.find(' ');
        std::string_view first = v.substr(0, separator);
        std::string_view rest = v.substr(separator + 1);

        if (first == "OPENMESH") {
            if (rest != "Version 1.1") {
                Log::Warning(fmt::format(
                    "Found {} in mesh '{}' but expected Version 1.1 so the loading might "
                    "misbehave", rest, path
                ));
            }
        }
        else if (first == "VERTICES") {
            data.nVertices = std::stoi(std::string(rest));
            data.vertices.reserve(data.nVertices);
        }
        else if (first == "FACES") {
            data.nFaces = std::stoi(std::string(rest));
            data.faces.reserve(data.nFaces);
        }
        else if (first == "MAPPING") {
            if (rest != "NORMALIZED") {
                Log::Warning(fmt::format(
                    "Found mapping '{}' in mesh '{}' but only 'NORMALIZED' is supported",
                    rest, path
                ));
            }
        }
        else if (first == "SAMPLING") {
            if (rest != "LINEAR") {
                Log::Warning(fmt::format(
                    "Found sampling '{}' in mesh '{}' but only 'LINEAR' is supported",
                    rest, path
                ));
            }
        }
        else if (first == "PROJECTION") {
            if (rest != "PERSPECTIVE") {
                Log::Warning(fmt::format(
                    "Found projection '{}' in mesh '{}' but only 'PERSPECTIVE' is "
                    "supported", rest, path
                ));
            }
        }
        else if (first == "ORTHO_LEFT") {
            data.ortho.left = std::stof(std::string(rest));
        }
        else if (first == "ORTHO_RIGHT") {
            data.ortho.right = std::stof(std::string(rest));
        }
        else if (first == "ORTHO_TOP") {
            data.ortho.top = std::stof(std::string(rest));
        }
        else if (first == "ORTHO_BOTTOM") {
            data.ortho.bottom = std::stof(std::string(rest));
        }
        else if (first == "PERSPECTIVE_XOFFSET") {
            data.perspective.offset.x = std::stof(std::string(rest));
            data.perspective.hasOffset = true;
        }
        else if (first == "PERSPECTIVE_YOFFSET") {
            data.perspective.offset.y = std::stof(std::string(rest));
            data.perspective.hasOffset = true;
        }
        else if (first == "PERSPECTIVE_ZOFFSET") {
            data.perspective.offset.z = std::stof(std::string(rest));
            data.perspective.hasOffset = true;
        }
        else if (first == "PERSPECTIVE_ROLL") {
            data.perspective.direction.roll = std::stof(std::string(rest));
        }
        else if (first == "PERSPECTIVE_PITCH") {
            data.perspective.direction.pitch = std::stof(std::string(rest));
        }
        else if (first == "PERSPECTIVE_YAW") {
            data.perspective.direction.yaw = std::stof(std::string(rest));
        }
        else if (first == "PERSPECTIVE_LEFT") {
            data.perspective.fov.left = std::stof(std::string(rest));
            data.perspective.hasFov = true;
        }
        else if (first == "PERSPECTIVE_RIGHT") {
            data.perspective.fov.right = std::stof(std::string(rest));
            data.perspective.hasFov = true;
        }
        else if (first == "PERSPECTIVE_TOP") {
            data.perspective.fov.top = std::stof(std::string(rest));
            data.perspective.hasFov = true;
        }
        else if (first == "PERSPECTIVE_BOTTOM") {
            data.perspective.fov.bottom = std::stof(std::string(rest));
            data.perspective.hasFov = true;
        }
        else if (first == "NATIVEXRES") {
            data.resolution.x = std::stoi(std::string(rest));
        }
        else if (first == "NATIVEYRES") {
            data.resolution.y = std::stoi(std::string(rest));
        }
        else if (first == "SUBVERSION") {
            int version = std::stoi(std::string(rest));
            if (version != 5) {
                Log::Warning(fmt::format(
                    "Found subversion {} in mesh '{}' but only version 5 is tested",
                    version, path
                ));
            }
        }
        else if (first == "GAMMA") {
            float gamma = std::stof(std::string(rest));
            if (gamma != data.gamma) {
                data.gamma = gamma;
                Log::Warning(fmt::format(
                    "Found GAMMA value of {} in mesh '{}' we do not support per-viewport "
                    "gamma values", data.gamma, path
                ));
            }
        }
        else if (first == "DO_NO_WARP") {
            data.doNotWarp = std::stoi(std::string(rest)) != 0;
        }
        else if (first == "USE_SPHERE_SAMPLE_COORDINATE_SYSTEM") {
            bool useSphereSampling = std::stoi(std::string(rest)) != 0;
            if (useSphereSampling) {
                Log::Warning(fmt::format(
                    "Found request to use Sphere Sample Coordinate System in mesh {} "
                    "but we do not support this", path
                ));
            }
        }
        else if (first == "FRUSTUM_EULER_ANGLES") {
            data.frustumEulerAngles.useAngles = std::stoi(std::string(rest)) != 0;
            if (data.frustumEulerAngles.useAngles) {
                Log::Warning(fmt::format(
                    "Enabled frustum euler angles in mesh '{}' but we do not know how "
                    "these work, yet", path
                ));
            }
        }
        else if (first == "FRUSTUM_EULER_YAW") {
            data.frustumEulerAngles.yaw = std::stof(std::string(rest));
        }
        else if (first == "FRUSTUM_EULER_PITCH") {
            data.frustumEulerAngles.pitch = std::stof(std::string(rest));
        }
        else if (first == "FRUSTUM_EULER_ROLL") {
            data.frustumEulerAngles.roll = std::stof(std::string(rest));
        }
        else if (first == "LABEL") {
            data.label = std::string(rest);
        }
        else if (first == "APPLY_MASK") {
            data.applyMask = std::stoi(std::string(rest));
            if (data.applyMask) {
                Log::Warning(fmt::format(
                    "Mesh '{}' requested to apply a mask. Currently this is handled "
                    "outside the mesh by specifying a 'mask' attribute on the 'Viewport' "
                    "instead", path
                ));
            }
        }
        else if (first == "APPLY_BLACK_LEVEL") {
            data.applyBlackLevel = std::stoi(std::string(rest));
            if (data.applyBlackLevel) {
                Log::Warning(fmt::format(
                    "Mesh '{}' requested to apply a blacklevel image. Currently this is "
                    "handled outside the mesh by specifying a 'BlackLevelMask' attribute "
                    "on the 'Viewport' instead", path
                ));
            }
        }
        else if (first == "APPLY_COLOR") {
            data.applyColor = std::stoi(std::string(rest));
            if (data.applyBlackLevel) {
                Log::Warning(fmt::format(
                    "Mesh '{}' requested to apply an overlay image. Currently this is "
                    "handled outside the mesh by specifying an 'overlay' attribute on "
                    "the 'Viewport' instead", path
                ));
            }
        }
        else if (first == "[") {
            // Face
            size_t sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2035,
                    fmt::format(
                        "Illegal formatting of face in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view f1 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2035,
                    fmt::format(
                        "Illegal formatting of face in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view f2 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2035,
                    fmt::format(
                        "Illegal formatting of face in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view f3 = rest.substr(0, sep);

            Data::Face f;
            f.f1 = static_cast<unsigned int>(std::stoi(std::string(f1)));
            f.f2 = static_cast<unsigned int>(std::stoi(std::string(f2)));
            f.f3 = static_cast<unsigned int>(std::stoi(std::string(f3)));
            data.faces.push_back(f);
        }
        else {
            // Nothing matched previously, so it has to be a vertex or an unknown key now
            try {
                // We try to convert the first value into a float.  If it succeeds, we
                // have reached the vertices.  Otherwise we have found an unknown key
                [[maybe_unused]] float dummy = std::stof(std::string(first));
            }
            catch (const std::invalid_argument&) {
                Log::Warning(fmt::format(
                    "Unknown key {} found in scalable mesh '{}'. Please report usage of "
                    "this key, preferably with an example, to the SGCT developers",
                    first, path
                ));
                continue;
            }


            std::string_view x = first;

            size_t sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2036,
                    fmt::format(
                        "Illegal formatting of vertex in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view y = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2036,
                    fmt::format(
                        "Illegal formatting of vertex in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view intensity = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2036,
                    fmt::format(
                        "Illegal formatting of vertex in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view s = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::Scalable, 2036,
                    fmt::format(
                        "Illegal formatting of vertex in file '{}' in line {}",
                        path, line
                    )
                );
            }
            std::string_view t = rest.substr(0, sep);

            Data::Vertex vertex;
            vertex.x = std::stof(std::string(x));
            vertex.y = std::stof(std::string(y));
            vertex.intensity = std::stoi(std::string(intensity));
            vertex.s = std::stof(std::string(s));
            vertex.t = std::stof(std::string(t));
            data.vertices.push_back(vertex);
        }
    }


    if (data.perspective.hasFov) {
        // pitch, yaw, roll.  degrees -> radians
        // if we don't have a direction, all these values will be 0 anyway
        glm::quat q(glm::vec3(
            glm::radians(data.perspective.direction.pitch),
            glm::radians(data.perspective.direction.yaw),
            glm::radians(data.perspective.direction.roll)
        ));

        parent.setViewPlaneCoordsUsingFOVs(
            data.perspective.fov.top,
            data.perspective.fov.bottom,
            data.perspective.fov.left,
            data.perspective.fov.right,
            quat(q.x, q.y, q.z, q.w)
        );
        Engine::instance().updateFrustums();
    }
    if (data.perspective.hasOffset) {
        parent.projectionPlane().offset(
            vec3{
                data.perspective.offset.x,
                data.perspective.offset.y,
                data.perspective.offset.z
            }
        );
    }
    if (data.nVertices != static_cast<int>(data.vertices.size()) ||
        data.nFaces != static_cast<int>(data.faces.size()))
    {
        throw Error(
            Error::Component::Scalable, 2061,
            fmt::format("Incorrect mesh data geometry in file '{}'", path)
        );
    }

    Buffer buf;
    buf.geometryType = GL_TRIANGLES;
    buf.vertices.reserve(data.vertices.size());
    for (const Data::Vertex& vertex : data.vertices) {
        Buffer::Vertex v;
        float x = (vertex.x / data.resolution.x) * parent.size().x + parent.position().x;
        float y = (vertex.y / data.resolution.y) * parent.size().y + parent.position().y;

        // Normalize vertices between 0 and 1
        float x2 = (x - data.ortho.left) / (data.ortho.right - data.ortho.left);
        float y2 = (y - data.ortho.bottom) / (data.ortho.top - data.ortho.bottom);

        // Normalize vertices to -1.0 to 1.0
        v.x = x2 * 2.f - 1.f;
        v.y = y2 * 2.f - 1.f;

        v.r = vertex.intensity / 255.f;
        v.g = vertex.intensity / 255.f;
        v.b = vertex.intensity / 255.f;
        v.a = 1.f;
        //v.s = (1.f - vertex.s) * parent.size().x + parent.position().x;
        v.s = (1.f - vertex.t) * parent.size().x + parent.position().x;
        //v.t = (1.f - vertex.t) * parent.size().x + parent.position().x;
        v.t = (1.f - vertex.s) * parent.size().x + parent.position().x;

        buf.vertices.push_back(v);
    }
    buf.indices.reserve(data.faces.size() * 3);
    for (const Data::Face& face : data.faces) {
        buf.indices.push_back(face.f1);
        buf.indices.push_back(face.f2);
        buf.indices.push_back(face.f3);
    }

    return buf;
}

} // namespace sgct::correction








