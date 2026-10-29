/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <sgct/config.h>
#include <sgct/glm.h>
#include <sgct/user.h>

using namespace sgct;
using Catch::Matchers::WithinAbs;

namespace {
    void checkVec(const glm::vec3& actual, const glm::vec3& expected) {
        CAPTURE(actual.x, actual.y, actual.z, expected.x, expected.y, expected.z);
        CHECK_THAT(actual.x, WithinAbs(expected.x, 1e-5));
        CHECK_THAT(actual.y, WithinAbs(expected.y, 1e-5));
        CHECK_THAT(actual.z, WithinAbs(expected.z, 1e-5));
    }

    // 90 degree rotation around the y axis, which maps +x to -z
    const glm::mat4 RotY90 =
        glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(0.f, 1.f, 0.f));
} // namespace

TEST_CASE("User: Default", "[user]") {
    const User user = User(config::User());
    CHECK(user.name() == "default");
    CHECK(user.eyeSeparation() == 0.06f);
    checkVec(user.posMono(), glm::vec3(0.f, 0.f, 0.f));
    checkVec(user.posLeftEye(), glm::vec3(-0.03f, 0.f, 0.f));
    checkVec(user.posRightEye(), glm::vec3(0.03f, 0.f, 0.f));
    CHECK(user.headTrackerName().empty());
    CHECK(user.headTrackerDeviceName().empty());
    CHECK_FALSE(user.isTracked());
}

TEST_CASE("User: Configuration", "[user]") {
    config::User conf;
    conf.name = "abc";
    conf.eyeSeparation = 0.1f;
    conf.position = glm::vec3(1.f, 2.f, 3.f);
    conf.tracking = config::User::Tracking{ .tracker = "tracker", .device = "device" };
    const User user = User(conf);

    CHECK(user.name() == "abc");
    CHECK(user.eyeSeparation() == 0.1f);
    checkVec(user.posMono(), glm::vec3(1.f, 2.f, 3.f));
    checkVec(user.posLeftEye(), glm::vec3(0.95f, 2.f, 3.f));
    checkVec(user.posRightEye(), glm::vec3(1.05f, 2.f, 3.f));
    CHECK(user.headTrackerName() == "tracker");
    CHECK(user.headTrackerDeviceName() == "device");
    CHECK(user.isTracked());
}

TEST_CASE("User: Configuration orientation rotates the eyes", "[user]") {
    config::User conf;
    conf.position = glm::vec3(0.f, 0.f, 4.f);
    conf.transformation = RotY90;
    const User user = User(conf);

    // The orientation must neither move the user's position nor be lost for the eyes
    checkVec(user.posMono(), glm::vec3(0.f, 0.f, 4.f));
    checkVec(user.posLeftEye(), glm::vec3(0.f, 0.f, 4.03f));
    checkVec(user.posRightEye(), glm::vec3(0.f, 0.f, 3.97f));
}

TEST_CASE("User: Tracking is incomplete", "[user]") {
    config::User conf;
    conf.tracking = config::User::Tracking{ .tracker = "tracker", .device = "" };
    User user = User(conf);
    CHECK_FALSE(user.isTracked());

    user.setHeadTracker("", "device");
    CHECK_FALSE(user.isTracked());

    user.setHeadTracker("tracker", "device");
    CHECK(user.isTracked());
    CHECK(user.headTrackerName() == "tracker");
    CHECK(user.headTrackerDeviceName() == "device");
}

TEST_CASE("User: setPos", "[user]") {
    User user = User(config::User());
    user.setPos(glm::vec3(1.f, 2.f, 3.f));
    checkVec(user.posMono(), glm::vec3(1.f, 2.f, 3.f));
    checkVec(user.posLeftEye(), glm::vec3(0.97f, 2.f, 3.f));
    checkVec(user.posRightEye(), glm::vec3(1.03f, 2.f, 3.f));
}

TEST_CASE("User: setEyeSeparation", "[user]") {
    User user = User(config::User());
    user.setPos(glm::vec3(1.f, 0.f, 0.f));
    user.setEyeSeparation(1.f);
    CHECK(user.eyeSeparation() == 1.f);
    checkVec(user.posMono(), glm::vec3(1.f, 0.f, 0.f));
    checkVec(user.posLeftEye(), glm::vec3(0.5f, 0.f, 0.f));
    checkVec(user.posRightEye(), glm::vec3(1.5f, 0.f, 0.f));

    user.setEyeSeparation(0.f);
    checkVec(user.posLeftEye(), glm::vec3(1.f, 0.f, 0.f));
    checkVec(user.posRightEye(), glm::vec3(1.f, 0.f, 0.f));
}

TEST_CASE("User: setOrientation with Euler angles", "[user]") {
    User user = User(config::User());
    user.setPos(glm::vec3(0.f, 0.f, 1.f));

    // Applying the same orientation multiple times must not move the user
    for (int i = 0; i < 3; i++) {
        user.setOrientation(0.f, 0.f, 0.f);
        checkVec(user.posMono(), glm::vec3(0.f, 0.f, 1.f));
        checkVec(user.posLeftEye(), glm::vec3(-0.03f, 0.f, 1.f));
        checkVec(user.posRightEye(), glm::vec3(0.03f, 0.f, 1.f));
    }

    for (int i = 0; i < 3; i++) {
        user.setOrientation(0.f, glm::half_pi<float>(), 0.f);
        checkVec(user.posMono(), glm::vec3(0.f, 0.f, 1.f));
        checkVec(user.posLeftEye(), glm::vec3(0.f, 0.f, 1.03f));
        checkVec(user.posRightEye(), glm::vec3(0.f, 0.f, 0.97f));
    }

    // Rotating around z tilts the eyes
    user.setOrientation(0.f, 0.f, glm::half_pi<float>());
    checkVec(user.posMono(), glm::vec3(0.f, 0.f, 1.f));
    checkVec(user.posLeftEye(), glm::vec3(0.f, -0.03f, 1.f));
    checkVec(user.posRightEye(), glm::vec3(0.f, 0.03f, 1.f));
}

TEST_CASE("User: setOrientation with quaternion", "[user]") {
    User user = User(config::User());
    user.setPos(glm::vec3(2.f, 0.f, 0.f));

    const glm::quat q = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.f, 1.f, 0.f));
    for (int i = 0; i < 3; i++) {
        user.setOrientation(q);
        checkVec(user.posMono(), glm::vec3(2.f, 0.f, 0.f));
        checkVec(user.posLeftEye(), glm::vec3(2.f, 0.f, 0.03f));
        checkVec(user.posRightEye(), glm::vec3(2.f, 0.f, -0.03f));
    }

    // Changing the position or eye separation retains the orientation
    user.setPos(glm::vec3(0.f, 1.f, 0.f));
    checkVec(user.posLeftEye(), glm::vec3(0.f, 1.f, 0.03f));
    checkVec(user.posRightEye(), glm::vec3(0.f, 1.f, -0.03f));
    user.setEyeSeparation(1.f);
    checkVec(user.posLeftEye(), glm::vec3(0.f, 1.f, 0.5f));
    checkVec(user.posRightEye(), glm::vec3(0.f, 1.f, -0.5f));
}

TEST_CASE("User: setTransform", "[user]") {
    User user = User(config::User());
    user.setPos(glm::vec3(0.f, 0.f, 1.f));

    const glm::mat4 transform =
        glm::translate(glm::mat4(1.f), glm::vec3(1.f, 2.f, 3.f)) * RotY90;
    for (int i = 0; i < 3; i++) {
        user.setTransform(transform);
        checkVec(user.posMono(), glm::vec3(1.f, 2.f, 4.f));
        checkVec(user.posLeftEye(), glm::vec3(1.f, 2.f, 4.03f));
        checkVec(user.posRightEye(), glm::vec3(1.f, 2.f, 3.97f));
    }

    user.setTransform(glm::mat4(1.f));
    checkVec(user.posMono(), glm::vec3(0.f, 0.f, 1.f));
    checkVec(user.posLeftEye(), glm::vec3(-0.03f, 0.f, 1.f));
    checkVec(user.posRightEye(), glm::vec3(0.03f, 0.f, 1.f));
}
