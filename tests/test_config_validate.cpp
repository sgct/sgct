/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <sgct/config.h>
#include <sgct/error.h>
#include "testhelpers.h"
#include <array>

using namespace sgct;
using namespace sgct::config;
using namespace sgct::test;

namespace {
    // Returns the smallest cluster that passes the validation
    Cluster minimalCluster() {
        Cluster cluster;
        cluster.masterAddress = "localhost";
        cluster.users.emplace_back();
        Node node;
        node.address = "localhost";
        node.port = 20401;
        node.windows.emplace_back();
        cluster.nodes.push_back(node);
        return cluster;
    }

    template <typename T>
    void checkValidateError(const T& value, int code) {
        checkError(
            [&]() {
                if constexpr (std::is_same_v<T, Cluster>) {
                    validateCluster(value);
                }
                else if constexpr (std::is_same_v<T, Viewport>) {
                    Cluster c = minimalCluster();
                    c.nodes[0].windows[0].viewports.push_back(value);
                    validateCluster(c);
                }
                else {
                    validateProjection(value);
                }
            },
            Error::Component::Config,
            code
        );
    }
} // namespace

TEST_CASE("ValidateCluster: Minimal cluster", "[validatecluster]") {
    CHECK_NOTHROW(validateCluster(minimalCluster()));
}

TEST_CASE("ValidateCluster: Cluster", "[validatecluster]") {
    SECTION("1120: Empty master address") {
        Cluster c = minimalCluster();
        c.masterAddress = "";
        checkValidateError(c, 1120);
    }

    SECTION("1122: No users") {
        Cluster c = minimalCluster();
        c.users.clear();
        checkValidateError(c, 1122);
    }

    SECTION("1123: Multiple unnamed users") {
        Cluster c = minimalCluster();
        c.users.emplace_back();
        checkValidateError(c, 1123);
    }

    SECTION("1127: No nodes") {
        Cluster c = minimalCluster();
        c.nodes.clear();
        checkValidateError(c, 1127);
    }
}

TEST_CASE("ValidateCluster: User names", "[validatecluster]") {
    Cluster c = minimalCluster();
    c.users.push_back(User{ .name = "a" });
    c.users.push_back(User{ .name = "b" });
    CHECK_NOTHROW(validateCluster(c));

    SECTION("1124: Adjacent duplicate") {
        c.users.push_back(User{ .name = "b" });
        checkValidateError(c, 1124);
    }

    SECTION("1124: Non-adjacent duplicate") {
        c.users.push_back(User{ .name = "a" });
        checkValidateError(c, 1124);
    }

    SECTION("1124: Duplicate before unnamed user") {
        c.users.push_back(User{ .name = "c" });
        c.users.insert(c.users.begin(), User{ .name = "c" });
        checkValidateError(c, 1124);
    }
}

TEST_CASE("ValidateCluster: Fisheye crop", "[validatecluster]") {
    using Crop = FisheyeProjection::Crop;

    SECTION("Valid") {
        const Crop crop = GENERATE(
            Crop{ .left = 0.f, .right = 0.f, .bottom = 0.f, .top = 0.f },
            // Cropping only one side
            Crop{ .left = 0.2f, .right = 0.f, .bottom = 0.f, .top = 0.f },
            Crop{ .left = 0.f, .right = 0.2f, .bottom = 0.f, .top = 0.f },
            Crop{ .left = 0.f, .right = 0.f, .bottom = 0.2f, .top = 0.f },
            Crop{ .left = 0.f, .right = 0.f, .bottom = 0.f, .top = 0.2f },
            // Cropping a 16:9 projector
            Crop{ .left = 0.f, .right = 0.f, .bottom = 0.21875f, .top = 0.21875f },
            // Touching but not overlapping
            Crop{ .left = 0.75f, .right = 0.25f, .bottom = 0.5f, .top = 0.5f }
        );
        CAPTURE(crop.left, crop.right, crop.bottom, crop.top);
        FisheyeProjection p;
        p.crop = crop;
        CHECK_NOTHROW(validateProjection(p));
    }

    SECTION("1061: Left and right overlap") {
        FisheyeProjection p;
        p.crop = Crop{ .left = 0.6f, .right = 0.7f, .bottom = 0.f, .top = 0.f };
        checkValidateError(p, 1061);
    }

    SECTION("1062: Bottom and top overlap") {
        FisheyeProjection p;
        p.crop = Crop{ .left = 0.f, .right = 0.f, .bottom = 0.7f, .top = 0.6f };
        checkValidateError(p, 1062);
    }
}

TEST_CASE("ValidateCluster: Node", "[validatecluster]") {
    SECTION("1110: Empty address") {
        Cluster c = minimalCluster();
        c.nodes[0].address = "";
        checkValidateError(c, 1110);
    }

    SECTION("1111: Zero port") {
        Cluster c = minimalCluster();
        c.nodes[0].port = 0;
        checkValidateError(c, 1111);
    }

    SECTION("1112: Zero data transfer port") {
        Cluster c = minimalCluster();
        c.nodes[0].dataTransferPort = 0;
        checkValidateError(c, 1112);
    }

    SECTION("1113: No windows") {
        Cluster c = minimalCluster();
        c.nodes[0].windows.clear();
        checkValidateError(c, 1113);
    }

    SECTION("1107: Negative window id") {
        Cluster c = minimalCluster();
        c.nodes[0].windows[0].id = -1;
        checkValidateError(c, 1107);
    }

    SECTION("1107: Duplicate window id") {
        Cluster c = minimalCluster();
        c.nodes[0].windows.emplace_back();
        checkValidateError(c, 1107);
    }
}

TEST_CASE("ValidateCluster: Blit window", "[validatecluster]") {
    Cluster c = minimalCluster();
    Window second;
    second.id = 1;
    c.nodes[0].windows.push_back(second);

    SECTION("Valid") {
        c.nodes[0].windows[1].blitWindowId = 0;
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("No blitting") {
        c.nodes[0].windows[1].blitWindowId = -1;
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1108: Negative id") {
        c.nodes[0].windows[1].blitWindowId = GENERATE(int8_t(-2), int8_t(-128));
        checkValidateError(c, 1108);
    }

    SECTION("1109: Missing window") {
        c.nodes[0].windows[1].blitWindowId = 2;
        checkValidateError(c, 1109);
    }

    SECTION("1114: Blit from itself") {
        c.nodes[0].windows[1].blitWindowId = 1;
        checkValidateError(c, 1114);
    }
}

TEST_CASE("ValidateCluster: User", "[validatecluster]") {
    SECTION("Valid") {
        Cluster c = minimalCluster();
        c.users[0].eyeSeparation = 0.f;
        c.users[0].name = "user";
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1000: Negative eye separation") {
        Cluster c = minimalCluster();
        c.users[0].eyeSeparation = -0.065f;
        checkValidateError(c, 1000);
    }

    SECTION("1001: Empty tracking device") {
        Cluster c = minimalCluster();
        c.users[0].tracking = User::Tracking{ .tracker = "tracker", .device = "" };
        checkValidateError(c, 1001);
    }

    SECTION("1002: Empty tracking tracker") {
        Cluster c = minimalCluster();
        c.users[0].tracking = User::Tracking{ .tracker = "", .device = "device" };
        checkValidateError(c, 1002);
    }

    SECTION("1003: Reserved name default") {
        Cluster c = minimalCluster();
        c.users[0].name = "default";
        checkValidateError(c, 1003);
    }
}

TEST_CASE("ValidateCluster: User tracking", "[validatecluster]") {
    Cluster c = minimalCluster();
    Tracker tracker;
    tracker.name = "tracker";
    Tracker::Device device;
    device.name = "device";
    device.sensors.push_back(Tracker::Device::Sensor{ .vrpnAddress = "head@localhost" });
    tracker.devices.push_back(device);
    c.trackers.push_back(tracker);

    SECTION("Valid") {
        c.users[0].tracking = User::Tracking{ .tracker = "tracker", .device = "device" };
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1125: Unknown tracker") {
        c.users[0].tracking = User::Tracking{ .tracker = "other", .device = "device" };
        checkValidateError(c, 1125);
    }

    SECTION("1126: Unknown device") {
        c.users[0].tracking = User::Tracking{ .tracker = "tracker", .device = "other" };
        checkValidateError(c, 1126);
    }
}

TEST_CASE("ValidateCluster: Capture", "[validatecluster]") {
    using Range = Capture::ScreenShotRange;

    SECTION("Valid") {
        const Range range = GENERATE(
            Range{ .first = -1, .last = -1 },
            Range{ .first = 2, .last = 5 },
            Range{ .first = 5, .last = -1 },
            Range{ .first = -1, .last = 5 }
        );
        CAPTURE(range.first, range.last);
        Cluster c = minimalCluster();
        c.capture = Capture{ .path = "screenshots", .range = range };
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1010: Empty path") {
        Cluster c = minimalCluster();
        c.capture = Capture{ .path = "" };
        checkValidateError(c, 1010);
    }

    SECTION("1011: Empty range") {
        Cluster c = minimalCluster();
        c.capture = Capture{ .range = Range{ .first = 5, .last = 5 } };
        checkValidateError(c, 1011);
    }

    SECTION("1011: Inverted range") {
        Cluster c = minimalCluster();
        c.capture = Capture{ .range = Range{ .first = 5, .last = 3 } };
        checkValidateError(c, 1011);
    }
}

TEST_CASE("ValidateCluster: Settings", "[validatecluster]") {
    Cluster c = minimalCluster();
    c.settings = Settings();
    c.settings->display = Settings::Display();

    SECTION("Valid") {
        c.settings->display->swapInterval = 0;
        c.settings->display->refreshRate = 0;
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1020: Negative swap interval") {
        c.settings->display->swapInterval = -1;
        checkValidateError(c, 1020);
    }

    SECTION("1021: Negative refresh rate") {
        c.settings->display->refreshRate = -1;
        checkValidateError(c, 1021);
    }
}

TEST_CASE("ValidateCluster: Tracker", "[validatecluster]") {
    Cluster c = minimalCluster();
    Tracker tracker;
    tracker.name = "tracker";
    Tracker::Device device;
    device.name = "device";
    device.sensors.push_back(Tracker::Device::Sensor{ .vrpnAddress = "a@localhost" });
    device.buttons.push_back(Tracker::Device::Button{ .vrpnAddress = "b@localhost" });
    device.axes.push_back(Tracker::Device::Axis{ .vrpnAddress = "c@localhost" });
    tracker.devices.push_back(device);
    c.trackers.push_back(tracker);

    SECTION("Valid") {
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1040: Empty tracker name") {
        c.trackers[0].name = "";
        checkValidateError(c, 1040);
    }

    SECTION("1030: Empty device name") {
        c.trackers[0].devices[0].name = "";
        checkValidateError(c, 1030);
    }

    SECTION("1031: Empty sensor address") {
        c.trackers[0].devices[0].sensors.push_back(Tracker::Device::Sensor());
        checkValidateError(c, 1031);
    }

    SECTION("1032: Empty button address") {
        c.trackers[0].devices[0].buttons.push_back(Tracker::Device::Button());
        checkValidateError(c, 1032);
    }

    SECTION("1033: Empty axis address") {
        c.trackers[0].devices[0].axes.push_back(Tracker::Device::Axis());
        checkValidateError(c, 1033);
    }
}

TEST_CASE("ValidateCluster: Planar projection", "[validatecluster]") {
    using FOV = PlanarProjection::FOV;
    const FOV valid = FOV{ .down = -30.f, .left = -40.f, .right = 40.f, .up = 30.f };

    SECTION("Valid") {
        PlanarProjection p;
        p.fov = valid;
        CHECK_NOTHROW(validateProjection(p));
    }

    SECTION("1050: Same up and down") {
        PlanarProjection p;
        p.fov = valid;
        p.fov.down = p.fov.up;
        checkValidateError(p, 1050);
    }

    SECTION("1051: Same left and right") {
        PlanarProjection p;
        p.fov = valid;
        p.fov.left = p.fov.right;
        checkValidateError(p, 1051);
    }

    SECTION("TextureMapped: Valid") {
        TextureMappedProjection p;
        p.fov = valid;
        CHECK_NOTHROW(validateProjection(p));
    }

    SECTION("TextureMapped: 1050: Same up and down") {
        TextureMappedProjection p;
        p.fov = valid;
        p.fov.down = p.fov.up;
        checkValidateError(p, 1050);
    }

    SECTION("TextureMapped: 1051: Same left and right") {
        TextureMappedProjection p;
        p.fov = valid;
        p.fov.left = p.fov.right;
        checkValidateError(p, 1051);
    }
}

TEST_CASE("ValidateCluster: Fisheye projection", "[validatecluster]") {
    SECTION("Valid") {
        FisheyeProjection p;
        p.fov = 180.f;
        p.quality = GENERATE(1, 256, 1024, 1536, 2048);
        p.diameter = 14.8f;
        p.background = glm::vec4(0.f, 0.f, 0.f, 1.f);
        CAPTURE(*p.quality);
        CHECK_NOTHROW(validateProjection(p));
    }

    SECTION("1060: Non-positive field of view") {
        FisheyeProjection p;
        p.fov = GENERATE(0.f, -180.f);
        checkValidateError(p, 1060);
    }

    SECTION("1063: Non-positive quality") {
        FisheyeProjection p;
        p.quality = GENERATE(0, -1024);
        checkValidateError(p, 1063);
    }

    SECTION("1064: Quality not a power of two") {
        FisheyeProjection p;
        p.quality = GENERATE(3, 1000, 1537, 3072);
        checkValidateError(p, 1064);
    }

    SECTION("1065: Non-positive diameter") {
        FisheyeProjection p;
        p.diameter = GENERATE(0.f, -1.f);
        checkValidateError(p, 1065);
    }

    SECTION("1066: Negative background component") {
        FisheyeProjection p;
        p.background = GENERATE(
            glm::vec4(-1.f, 0.f, 0.f, 1.f),
            glm::vec4(0.f, -1.f, 0.f, 1.f),
            glm::vec4(0.f, 0.f, -1.f, 1.f),
            glm::vec4(0.f, 0.f, 0.f, -1.f)
        );
        checkValidateError(p, 1066);
    }
}

TEST_CASE("ValidateCluster: Spherical mirror projection", "[validatecluster]") {
    const SphericalMirrorProjection::Mesh mesh = {
        .bottom = "b.obj",
        .left = "l.obj",
        .right = "r.obj",
        .top = "t.obj"
    };

    SECTION("Valid") {
        SphericalMirrorProjection p;
        p.mesh = mesh;
        p.quality = GENERATE(1, 256, 2048);
        p.background = glm::vec4(0.f, 0.f, 0.f, 1.f);
        CAPTURE(*p.quality);
        CHECK_NOTHROW(validateProjection(p));
    }

    SECTION("1070: Non-positive quality") {
        SphericalMirrorProjection p;
        p.mesh = mesh;
        p.quality = GENERATE(0, -256);
        checkValidateError(p, 1070);
    }

    SECTION("1071: Quality not a power of two") {
        SphericalMirrorProjection p;
        p.mesh = mesh;
        p.quality = GENERATE(3, 1000, 1536);
        checkValidateError(p, 1071);
    }

    SECTION("1072: Negative background component") {
        SphericalMirrorProjection p;
        p.mesh = mesh;
        p.background = GENERATE(
            glm::vec4(-1.f, 0.f, 0.f, 1.f),
            glm::vec4(0.f, -1.f, 0.f, 1.f),
            glm::vec4(0.f, 0.f, -1.f, 1.f),
            glm::vec4(0.f, 0.f, 0.f, -1.f)
        );
        checkValidateError(p, 1072);
    }

    SECTION("1073: Empty mesh path") {
        SphericalMirrorProjection p;
        p.mesh = mesh;
        const int empty = GENERATE(0, 1, 2, 3);
        CAPTURE(empty);
        std::array<std::string*, 4> paths = {
            &p.mesh.bottom, &p.mesh.left, &p.mesh.right, &p.mesh.top
        };
        paths[empty]->clear();
        checkValidateError(p, 1073);
    }
}

TEST_CASE("ValidateCluster: Cubemap projection", "[validatecluster]") {
    SECTION("Valid") {
        CubemapProjection p;
        p.quality = 1024;
        CHECK_NOTHROW(validateProjection(p));
    }

    SECTION("1080: Non-positive quality") {
        CubemapProjection p;
        p.quality = GENERATE(0, -1024);
        checkValidateError(p, 1080);
    }
}

TEST_CASE("ValidateCluster: Viewport", "[validatecluster]") {
    SECTION("Valid") {
        Viewport v;
        v.user = "user";
        v.overlayTexture = "overlay.png";
        v.blendMaskTexture = "blend.png";
        v.blackLevelMaskTexture = "black.png";
        v.correctionMeshTexture = "mesh.pfm";
        Cluster c = minimalCluster();
        c.nodes[0].windows[0].viewports.push_back(v);
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1090: Empty user") {
        Viewport v;
        v.user = "";
        checkValidateError(v, 1090);
    }

    SECTION("1091: Empty overlay texture") {
        Viewport v;
        v.overlayTexture = "";
        checkValidateError(v, 1091);
    }

    SECTION("1092: Empty blend mask texture") {
        Viewport v;
        v.blendMaskTexture = "";
        checkValidateError(v, 1092);
    }

    SECTION("1093: Empty black level mask texture") {
        Viewport v;
        v.blackLevelMaskTexture = "";
        checkValidateError(v, 1093);
    }

    SECTION("1094: Empty correction mesh texture") {
        Viewport v;
        v.correctionMeshTexture = "";
        checkValidateError(v, 1094);
    }

    SECTION("Projection is validated") {
        Viewport v;
        v.projection = CubemapProjection{ .quality = 0 };
        checkValidateError(v, 1080);
    }
}

TEST_CASE("ValidateCluster: Window", "[validatecluster]") {
    SECTION("Valid") {
        Cluster c = minimalCluster();
        c.nodes[0].windows[0].name = "window";
        c.nodes[0].windows[0].tags = { "a", "b" };
        CHECK_NOTHROW(validateCluster(c));
    }

    SECTION("1100: Empty name") {
        Cluster c = minimalCluster();
        c.nodes[0].windows[0].name = "";
        checkValidateError(c, 1100);
    }

    SECTION("1101: Empty tag") {
        Cluster c = minimalCluster();
        c.nodes[0].windows[0].tags = { "a", "" };
        checkValidateError(c, 1101);
    }
}

TEST_CASE("GeneratorVersion: versionCheck", "[generatorversion]") {
    const GeneratorVersion v = GeneratorVersion{ .name = "gen", .major = 2, .minor = 3 };
    auto check = [&v](std::string name, int major, int minor) {
        return v.versionCheck(GeneratorVersion{ std::move(name), major, minor });
    };

    CHECK(check("gen", 2, 3));
    CHECK(check("gen", 2, 0));
    CHECK(check("gen", 1, 9));
    CHECK_FALSE(check("gen", 2, 4));
    CHECK_FALSE(check("gen", 3, 0));
    CHECK_FALSE(check("other", 1, 0));
}

TEST_CASE("GeneratorVersion: versionString", "[generatorversion]") {
    const GeneratorVersion v = GeneratorVersion{ .name = "gen", .major = 2, .minor = 3 };
    CHECK(v.versionString() == "gen 2.3");
    CHECK(GeneratorVersion().versionString() == " 0.0");
}
