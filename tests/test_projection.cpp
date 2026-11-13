/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <sgct/glm.h>
#include <sgct/projection.h>
#include <sgct/projection/projectionplane.h>

using namespace sgct;
using Catch::Matchers::WithinAbs;

namespace {
    void checkMatrix(const glm::mat4& actual, const glm::mat4& expected) {
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                CAPTURE(c, r);
                CHECK_THAT(actual[c][r], WithinAbs(expected[c][r], 1e-5));
            }
        }
    }

    // Transforms the point p with the matrix m and returns the normalized device
    // coordinates
    glm::vec3 toNdc(const glm::mat4& m, const glm::vec3& p) {
        const glm::vec4 clip = m * glm::vec4(p, 1.f);
        return glm::vec3(clip) / clip.w;
    }
} // namespace

TEST_CASE("ProjectionPlane: Default", "[projection]") {
    const ProjectionPlane plane;
    CHECK(plane.coordinateLowerLeft() == glm::vec3(-1.f, -1.f, -2.f));
    CHECK(plane.coordinateUpperLeft() == glm::vec3(-1.f, 1.f, -2.f));
    CHECK(plane.coordinateUpperRight() == glm::vec3(1.f, 1.f, -2.f));
}

TEST_CASE("ProjectionPlane: Set coordinates and offset", "[projection]") {
    ProjectionPlane plane;
    plane.setCoordinates(
        glm::vec3(-2.f, -1.f, -3.f),
        glm::vec3(-2.f, 1.f, -3.f),
        glm::vec3(2.f, 1.f, -3.f)
    );
    CHECK(plane.coordinateLowerLeft() == glm::vec3(-2.f, -1.f, -3.f));
    CHECK(plane.coordinateUpperLeft() == glm::vec3(-2.f, 1.f, -3.f));
    CHECK(plane.coordinateUpperRight() == glm::vec3(2.f, 1.f, -3.f));

    plane.offset(glm::vec3(1.f, 2.f, 3.f));
    CHECK(plane.coordinateLowerLeft() == glm::vec3(-1.f, 1.f, 0.f));
    CHECK(plane.coordinateUpperLeft() == glm::vec3(-1.f, 3.f, 0.f));
    CHECK(plane.coordinateUpperRight() == glm::vec3(3.f, 3.f, 0.f));
}

TEST_CASE("Projection: Default", "[projection]") {
    const Projection proj;
    checkMatrix(proj.viewMatrix(), glm::mat4(1.f));
    checkMatrix(proj.projectionMatrix(), glm::mat4(1.f));
    checkMatrix(proj.viewProjectionMatrix(), glm::mat4(1.f));
}

TEST_CASE("Projection: Centered", "[projection]") {
    // The plane is 2 units away, so the near plane at distance 1 is half its size
    const ProjectionPlane plane;
    Projection proj;
    proj.calculateProjection(glm::vec3(0.f), plane, 1.f, 100.f);

    checkMatrix(proj.viewMatrix(), glm::mat4(1.f));
    checkMatrix(
        proj.projectionMatrix(),
        glm::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.f, 100.f)
    );
    checkMatrix(
        proj.viewProjectionMatrix(),
        proj.projectionMatrix() * proj.viewMatrix()
    );
}

TEST_CASE("Projection: Off-axis", "[projection]") {
    const ProjectionPlane plane;
    Projection proj;
    proj.calculateProjection(glm::vec3(0.5f, -0.5f, 0.f), plane, 1.f, 100.f);

    checkMatrix(
        proj.viewMatrix(),
        glm::translate(glm::mat4(1.f), glm::vec3(-0.5f, 0.5f, 0.f))
    );
    checkMatrix(
        proj.projectionMatrix(),
        glm::frustum(-0.75f, 0.25f, -0.25f, 0.75f, 1.f, 100.f)
    );
}

TEST_CASE("Projection: Offset only moves the camera", "[projection]") {
    const ProjectionPlane plane;
    Projection proj;
    proj.calculateProjection(glm::vec3(0.f), plane, 1.f, 100.f, glm::vec3(1.f, 2.f, 3.f));

    checkMatrix(
        proj.viewMatrix(),
        glm::translate(glm::mat4(1.f), glm::vec3(-1.f, -2.f, -3.f))
    );
    checkMatrix(
        proj.projectionMatrix(),
        glm::frustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.f, 100.f)
    );
}

TEST_CASE("Projection: Plane corners map to the viewport corners", "[projection]") {
    const glm::vec3 eye = GENERATE(
        glm::vec3(0.f, 0.f, 0.f),
        glm::vec3(0.3f, -0.2f, 0.5f),
        glm::vec3(-1.5f, 0.75f, -0.5f)
    );
    const glm::vec3 axis = GENERATE(
        glm::vec3(0.f, 1.f, 0.f),
        glm::vec3(1.f, 0.f, 0.f),
        glm::normalize(glm::vec3(1.f, 2.f, 3.f))
    );
    const float angle = GENERATE(0.f, 30.f, 90.f, -135.f);
    CAPTURE(eye.x, eye.y, eye.z, axis.x, axis.y, axis.z, angle);

    // Rotate the default plane and the eye position around the origin
    const glm::mat4 rotation = glm::rotate(glm::mat4(1.f), glm::radians(angle), axis);
    const glm::mat3 rot = glm::mat3(rotation);
    ProjectionPlane plane;
    plane.setCoordinates(
        rot * glm::vec3(-1.f, -1.f, -2.f),
        rot * glm::vec3(-1.f, 1.f, -2.f),
        rot * glm::vec3(1.f, 1.f, -2.f)
    );
    const glm::vec3 base = rot * eye;
    const glm::vec3 lowerRight = rot * glm::vec3(1.f, -1.f, -2.f);

    Projection proj;
    proj.calculateProjection(base, plane, 0.1f, 50.f);
    const glm::mat4& vp = proj.viewProjectionMatrix();

    const glm::vec3 ll = toNdc(vp, plane.coordinateLowerLeft());
    CHECK_THAT(ll.x, WithinAbs(-1.f, 1e-4));
    CHECK_THAT(ll.y, WithinAbs(-1.f, 1e-4));
    const glm::vec3 ul = toNdc(vp, plane.coordinateUpperLeft());
    CHECK_THAT(ul.x, WithinAbs(-1.f, 1e-4));
    CHECK_THAT(ul.y, WithinAbs(1.f, 1e-4));
    const glm::vec3 ur = toNdc(vp, plane.coordinateUpperRight());
    CHECK_THAT(ur.x, WithinAbs(1.f, 1e-4));
    CHECK_THAT(ur.y, WithinAbs(1.f, 1e-4));
    const glm::vec3 lr = toNdc(vp, lowerRight);
    CHECK_THAT(lr.x, WithinAbs(1.f, 1e-4));
    CHECK_THAT(lr.y, WithinAbs(-1.f, 1e-4));

    // All corners are in the same plane, so they have the same depth
    CHECK_THAT(ll.z, WithinAbs(ur.z, 1e-4));
    CHECK(ll.z > -1.f);
    CHECK(ll.z < 1.f);
}

TEST_CASE("Projection: Near and far planes", "[projection]") {
    const ProjectionPlane plane;
    Projection proj;
    proj.calculateProjection(glm::vec3(0.f), plane, 0.5f, 20.f);
    const glm::mat4& vp = proj.viewProjectionMatrix();

    CHECK_THAT(toNdc(vp, glm::vec3(0.f, 0.f, -0.5f)).z, WithinAbs(-1.f, 1e-5));
    CHECK_THAT(toNdc(vp, glm::vec3(0.f, 0.f, -20.f)).z, WithinAbs(1.f, 1e-4));
}
