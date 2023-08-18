/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/skyskan.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <cassert>
#include <fstream>

namespace {
    struct Position {
        float x = 0.f;
        float y = 0.f;
    };
    struct Texture {
        float s = 0.f;
        float t = 0.f;
    };
    struct Face {
        int f1 = 0;
        int f2 = 0;
        int f3 = 0;
    };
} // namespace

namespace sgct::correction {

Buffer generateOBJMesh(const std::filesystem::path& path) {
    ZoneScoped;

    Log::Info(fmt::format("Reading Wavefront OBJ mesh data from {}", path));

    std::ifstream file(path);
    if (!file.good()) {
        throw Error(
            Error::Component::OBJ, 2030, fmt::format("Failed to open {}", path)
        );
    }

    std::vector<Position> positions;
    std::vector<Texture> texCoords;
    std::vector<Face> faces;

    std::vector<std::string> reported;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::string_view v = line;
        size_t separator = v.find(' ');
        std::string_view first = v.substr(0, separator);
        std::string_view rest = v.substr(separator + 1);

        if (first == "v") {
            size_t sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::OBJ, 2034,
                    fmt::format(
                        "Illegal vertex format in OBJ file {} in line '{}'", path, line
                    )
                );
            }
            std::string_view v1 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::OBJ, 2034,
                    fmt::format(
                        "Illegal vertex format in OBJ file {} in line '{}'", path, line
                    )
                );
            }
            std::string_view v2 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            std::string_view v3 = rest;
            float z = std::stof(std::string(v3));
            if (z != 0.f) {
                Log::Warning(fmt::format(
                    "Vertex in {} was using z coordinate which is not supported", path
                ));
            }

            Position p;
            p.x = std::stof(std::string(v1));
            p.y = std::stof(std::string(v2));
            positions.push_back(p);
        }
        else if (first == "vt") {
            size_t sep = rest.find(' ');
            std::string_view v1 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            std::string_view v2 = rest.substr(0, sep);

            Texture t;
            t.s = std::stof(std::string(v1));
            t.t = std::stof(std::string(v2));
            texCoords.push_back(t);
        }
        else if (first == "f") {
            size_t sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::OBJ, 2035,
                    fmt::format(
                        "Illegal face format in OBJ file {} in line '{}'", path, line
                    )
                );
            }
            std::string_view f1 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            if (sep == std::string_view::npos) {
                throw Error(
                    Error::Component::OBJ, 2035,
                    fmt::format(
                        "Illegal face format in OBJ file {} in line '{}'", path, line
                    )
                );
            }
            std::string_view f2 = rest.substr(0, sep);
            rest = rest.substr(sep + 1);

            sep = rest.find(' ');
            std::string_view f3 =
                sep == std::string_view::npos ?
                rest :
                rest.substr(0, sep);

            // The face description might just consist of a single value, in which case
            // the f.find method will return npos, which tells substr to "extract" the
            // entire string -> et voila; it still works
            Face f;
            f.f1 = std::stoi(std::string(f1.substr(0, f1.find('/'))));
            f.f2 = std::stoi(std::string(f2.substr(0, f2.find('/'))));
            f.f3 = std::stoi(std::string(f3.substr(0, f3.find('/'))));
            faces.push_back(f);
        }
        else if (first == "vn") {
            if (std::find(reported.begin(), reported.end(), "vn") == reported.end()) {
                Log::Warning(fmt::format("Ignoring normals in mesh {}", path));
                reported.push_back("vn");
            }
        }
        else if (first == "vp") {
            if (std::find(reported.begin(), reported.end(), "vp") == reported.end()) {
                Log::Warning(
                    fmt::format("Ignoring parameter space values in mesh {}", path)
                );
                reported.push_back("vp");
            }
        }
        else if (first == "l") {
            if (std::find(reported.begin(), reported.end(), "l") == reported.end()) {
                Log::Warning(fmt::format("Ignoring line elements in mesh {}", path));
                reported.push_back("l");
            }
        }
        else if (first == "mtllib") {
            if (std::find(reported.begin(), reported.end(), "mtllib") == reported.end()) {
                Log::Warning(fmt::format("Ignoring material library in mesh {}", path));
                reported.push_back("mtllib");
            }
        }
        else if (first == "usemtl") {
            if (std::find(reported.begin(), reported.end(), "usemtl") == reported.end()) {
                Log::Warning(
                    fmt::format("Ignoring material specification in mesh {}", path)
                );
                reported.push_back("usemtl");
            }
        }
        else if (first == "o") {
            if (std::find(reported.begin(), reported.end(), "o") == reported.end()) {
                Log::Warning(
                    fmt::format("Ignoring object specification in mesh {}", path)
                );
                reported.push_back("o");
            }
        }
        else if (first == "g") {
            if (std::find(reported.begin(), reported.end(), "g") == reported.end()) {
                Log::Warning(
                    fmt::format("Ignoring object group specification in mesh {}", path)
                );
                reported.push_back("g");
            }
        }
        else if (first == "s") {
            if (std::find(reported.begin(), reported.end(), "s") == reported.end()) {
                Log::Warning(
                    fmt::format("Ignoring shading specification in mesh {}", path)
                );
                reported.push_back("s");
            }
        }
        else {
            if (std::find(reported.begin(), reported.end(), first) == reported.end()) {
                Log::Warning(fmt::format(
                    "Encounted unsupported value type '{}' in mesh {}", first, path
                ));
                reported.push_back(std::string(first));
            }
        }
    }

    if (positions.size() != texCoords.size()) {
        throw Error(
            Error::Component::OBJ, 2031,
            fmt::format(
                "Vertex count doesn't match number of texture coordinates in {}", path
            )
        );
    }
    for (const Face& f : faces) {
        const int nPositions = static_cast<int>(positions.size());
        // OBJ uses 1-based indices, so we need to allow for one bigger than the number
        // of positions
        bool invalid = f.f1 > nPositions || f.f2 > nPositions || f.f3 > nPositions;
        if (invalid) {
            throw Error(
                Error::Component::OBJ, 2032,
                fmt::format(
                    "Faces in mesh {} referenced vertices that were undefined", path
                )
            );
        }

        if (f.f1 < 0 || f.f2 < 0 || f.f3 < 0) {
            throw Error(
                Error::Component::OBJ, 2033,
                fmt::format(
                    "Faces in mesh {} are using relative index positions that are "
                    "unsupported", path
                )
            );
        }
    }

    Buffer buffer;
    buffer.geometryType = GL_TRIANGLES;
    buffer.vertices.reserve(positions.size());
    assert(positions.size() == texCoords.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        Buffer::Vertex v;
        v.x = positions[i].x;
        v.y = positions[i].y;
        v.r = 1.f;
        v.g = 1.f;
        v.b = 1.f;
        v.a = 1.f;
        v.s = texCoords[i].s;
        v.t = texCoords[i].t;
        buffer.vertices.push_back(v);

    }
    buffer.indices.reserve(faces.size() * 3);
    for (const Face& f : faces) {
        // 1-based indexing vs 0-based indexing
        buffer.indices.push_back(static_cast<unsigned int>(f.f1 - 1));
        buffer.indices.push_back(static_cast<unsigned int>(f.f2 - 1));
        buffer.indices.push_back(static_cast<unsigned int>(f.f3 - 1));
    }

    return buffer;
}

} // namespace sgct::correction
