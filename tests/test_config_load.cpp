/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/readconfig.h>
#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::quat fromEuler(float yaw, float pitch, float roll) {
        glm::quat quat = glm::quat(1.0, 0.0, 0.0, 0.0);
        quat = glm::rotate(quat, glm::radians(-yaw), glm::vec3(0.0, 1.0, 0.0));
        quat = glm::rotate(quat, glm::radians(pitch), glm::vec3(1.0, 0.0, 0.0));
        quat = glm::rotate(quat, glm::radians(roll), glm::vec3(0.0, 0.0, 1.0));

        sgct::quat res;
        std::memcpy(&res, glm::value_ptr(quat), sizeof(sgct::quat));
        return res;
    }
} // namespace

TEST_CASE("Load: 3DTV", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(std::string(BASE_PATH) + "/config/3DTV.json");

    CHECK(res.masterAddress == "localhost");
 
    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20400);
    
    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == true);
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::SideBySide);
    CHECK(w.size.x == 1920);
    CHECK(w.size.y == 1080);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
    const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
    CHECK(p.lowerLeft.x == -1.7156f);
    CHECK(p.lowerLeft.y == -0.965f);
    CHECK(p.lowerLeft.z == 0.f);
    CHECK(p.upperLeft.x == -1.7156f);
    CHECK(p.upperLeft.y == 0.965f);
    CHECK(p.upperLeft.z == 0.f);
    CHECK(p.upperRight.x == 1.7156f);
    CHECK(p.upperRight.y == 0.965f);
    CHECK(p.upperRight.z == 0.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: Kinect", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(std::string(BASE_PATH) + "/config/Kinect.json");

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.trackers.size() == 1);
    const Tracker& t = res.trackers[0];
    CHECK(t.name == "Kinect0");
    REQUIRE(t.devices.size() == 24);
    {
        const Device& d = t.devices[0];
        CHECK(d.name == "Head");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 0);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }

    {
        const Device& d = t.devices[1];
        CHECK(d.name == "Neck");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 1);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }

    {
        const Device& d = t.devices[2];
        CHECK(d.name == "Torso");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 2);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[3];
        CHECK(d.name == "Waist");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 3);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[4];
        CHECK(d.name == "Left Collar");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 4);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[5];
        CHECK(d.name == "Left Shoulder");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 5);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[6];
        CHECK(d.name == "Left Elbow");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 6);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[7];
        CHECK(d.name == "Left Wrist");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 7);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[8];
        CHECK(d.name == "Left Hand");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 8);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[9];
        CHECK(d.name == "Left Fingertip");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 9);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[10];
        CHECK(d.name == "Right Collar");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 10);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[11];
        CHECK(d.name == "Right Shoulder");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 11);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[12];
        CHECK(d.name == "Right Elbow");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 12);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[13];
        CHECK(d.name == "Right Wrist");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 13);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[14];
        CHECK(d.name == "Right Hand");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 14);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[15];
        CHECK(d.name == "Right Fingertip");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 15);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[16];
        CHECK(d.name == "Left Hip");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 16);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[17];
        CHECK(d.name == "Left Knee");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 17);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[18];
        CHECK(d.name == "Left Ankle");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 18);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[19];
        CHECK(d.name == "Left Foot");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 19);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[20];
        CHECK(d.name == "Right Hip");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 20);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[21];
        CHECK(d.name == "Right Knee");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 21);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[22];
        CHECK(d.name == "Right Ankle");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 22);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }
    {
        const Device& d = t.devices[23];
        CHECK(d.name == "Right Foot");
        REQUIRE(d.sensors.size() == 1);
        const Device::Sensors& s = d.sensors[0];
        CHECK(s.identifier == 23);
        CHECK(s.vrpnAddress == "Tracker0@localhost");
    }

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 4);
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    CHECK(w.size.x == 960);
    CHECK(w.size.y == 540);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
    const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
    CHECK(p.lowerLeft.x == -1.778f);
    CHECK(p.lowerLeft.y == -1.f);
    CHECK(p.lowerLeft.z == 0.f);
    CHECK(p.upperLeft.x == -1.778f);
    CHECK(p.upperLeft.y == 1.f);
    CHECK(p.upperLeft.z == 0.f);
    CHECK(p.upperRight.x == 1.778f);
    CHECK(p.upperRight.y == 1.f);
    CHECK(p.upperRight.z == 0.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 1.5f);
}

TEST_CASE("Load: Multi Window", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(std::string(BASE_PATH) + "/config/multi_window.json");

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 5);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 4);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 0);
        CHECK(w.pos->y == 30);
        CHECK(w.size.x == 960);
        CHECK(w.size.y == 540);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
        const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
        CHECK(p.lowerLeft.x == -1.778f);
        CHECK(p.lowerLeft.y == 0.f);
        CHECK(p.lowerLeft.z == 0.f);
        CHECK(p.upperLeft.x == -1.778f);
        CHECK(p.upperLeft.y == 1.f);
        CHECK(p.upperLeft.z == 0.f);
        CHECK(p.upperRight.x == 0.f);
        CHECK(p.upperRight.y == 1.f);
        CHECK(p.upperRight.z == 0.f);
    }

    {
        const Window& w = n.windows[1];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 4);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 0);
        CHECK(w.pos->y == 570);
        CHECK(w.size.x == 480);
        CHECK(w.size.y == 540);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
        const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
        CHECK(p.lowerLeft.x == -1.778f);
        CHECK(p.lowerLeft.y == -1.f);
        CHECK(p.lowerLeft.z == 0.f);
        CHECK(p.upperLeft.x == -1.778f);
        CHECK(p.upperLeft.y == 0.f);
        CHECK(p.upperLeft.z == 0.f);
        CHECK(p.upperRight.x == -0.889f);
        CHECK(p.upperRight.y == 0.f);
        CHECK(p.upperRight.z == 0.f);
    }

    {
        const Window& w = n.windows[2];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 4);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 480);
        CHECK(w.pos->y == 570);
        CHECK(w.size.x == 480);
        CHECK(w.size.y == 270);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
        const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
        CHECK(p.lowerLeft.x == -0.889f);
        CHECK(p.lowerLeft.y == -0.5f);
        CHECK(p.lowerLeft.z == 0.f);
        CHECK(p.upperLeft.x == -0.889f);
        CHECK(p.upperLeft.y == 0.f);
        CHECK(p.upperLeft.z == 0.f);
        CHECK(p.upperRight.x == 0.f);
        CHECK(p.upperRight.y == 0.f);
        CHECK(p.upperRight.z == 0.f);
    }

    {
        const Window& w = n.windows[3];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 4);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 480);
        CHECK(w.pos->y == 840);
        CHECK(w.size.x == 480);
        CHECK(w.size.y == 270);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
        const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
        CHECK(p.lowerLeft.x == -0.889f);
        CHECK(p.lowerLeft.y == -1.f);
        CHECK(p.lowerLeft.z == 0.f);
        CHECK(p.upperLeft.x == -0.889f);
        CHECK(p.upperLeft.y == -0.5f);
        CHECK(p.upperLeft.z == 0.f);
        CHECK(p.upperRight.x == 0.f);
        CHECK(p.upperRight.y == -0.5f);
        CHECK(p.upperRight.z == 0.f);
    }

    {
        const Window& w = n.windows[4];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 4);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 1000);
        CHECK(w.pos->y == 30);
        CHECK(w.size.x == 960);
        CHECK(w.size.y == 1080);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<ProjectionPlane>(v.projection));
        const ProjectionPlane& p = std::get<ProjectionPlane>(v.projection);
        CHECK(p.lowerLeft.x == 0.f);
        CHECK(p.lowerLeft.y == -1.f);
        CHECK(p.lowerLeft.z == 0.f);
        CHECK(p.upperLeft.x == 0.f);
        CHECK(p.upperLeft.y == 1.f);
        CHECK(p.upperLeft.z == 0.f);
        CHECK(p.upperRight.x == 1.778f);
        CHECK(p.upperRight.y == 1.f);
        CHECK(p.upperRight.z == 0.f);
    }


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: Single Cylindrical", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_cylindrical.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<CylindricalProjection>(v.projection));
    const CylindricalProjection& p = std::get<CylindricalProjection>(v.projection);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 4096);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Single Equirectangular", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_equirectangular.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<EquirectangularProjection>(v.projection));
    const EquirectangularProjection& p =
        std::get<EquirectangularProjection>(v.projection);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 1024);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Single Fisheye FXAA", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_fisheye_fxaa.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.useFxaa.has_value());
    CHECK(*w.useFxaa == true);
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    CHECK(w.size.x == 512);
    CHECK(w.size.y == 512);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<FisheyeProjection>(v.projection));
    const FisheyeProjection& p = std::get<FisheyeProjection>(v.projection);
    REQUIRE(p.fov.has_value());
    CHECK(*p.fov == 180.f);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 512);
    REQUIRE(p.tilt.has_value());
    CHECK(*p.tilt == 0.f);
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 0.1f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Single Fisheye", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_fisheye.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    CHECK(w.size.x == 512);
    CHECK(w.size.y == 512);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<FisheyeProjection>(v.projection));
    const FisheyeProjection& p = std::get<FisheyeProjection>(v.projection);
    REQUIRE(p.fov.has_value());
    CHECK(*p.fov == 180.f);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 512);
    REQUIRE(p.tilt.has_value());
    CHECK(*p.tilt == 0.f);
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 0.1f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Single SBS Stereo", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_sbs_stereo.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::SideBySide);
    REQUIRE(w.pos.has_value());
    CHECK(w.pos->x == 200);
    CHECK(w.pos->y == 300);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 360);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
    const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
    CHECK(p.fov.left == -80.f / 2.f);
    CHECK(p.fov.right == 80.f / 2.f);
    CHECK(p.fov.up == 50.534015846724f / 2.f);
    CHECK(p.fov.down == -50.534015846724f / 2.f);
    REQUIRE(p.orientation.has_value());
    CHECK(p.orientation->x == 0.f);
    CHECK(p.orientation->y == 0.f);
    CHECK(p.orientation->z == 0.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: Single Two Win 3D", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_two_win_3D.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        CHECK(w.id == 1);
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "3D Rendering");
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 1);
        REQUIRE(w.draw2D.has_value());
        CHECK(*w.draw2D == false);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 10);
        CHECK(w.pos->y == 100);
        CHECK(w.size.x == 640);
        CHECK(w.size.y == 480);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        CHECK(p.orientation->x == 0.f);
        CHECK(p.orientation->y == 0.f);
        CHECK(p.orientation->z == 0.f);
    }

    {
        const Window& w = n.windows[1];
        CHECK(w.id == 0);
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "2D Overlay");
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 1);
        REQUIRE(w.draw3D.has_value());
        CHECK(*w.draw3D == false);
        REQUIRE(w.blitWindowId.has_value());
        CHECK(w.blitWindowId == 1);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 640);
        CHECK(w.pos->y == 100);
        CHECK(w.size.x == 640);
        CHECK(w.size.y == 480);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        CHECK(p.orientation->x == 0.f);
        CHECK(p.orientation->y == 0.f);
        CHECK(p.orientation->z == 0.f);
    }


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: Single Two Win", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/single_two_win.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 1);
        REQUIRE(w.isDecorated.has_value());
        CHECK(*w.isDecorated == true);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 10);
        CHECK(w.pos->y == 100);
        CHECK(w.size.x == 640);
        CHECK(w.size.y == 480);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(-20.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }

    {
        const Window& w = n.windows[1];
        REQUIRE(w.msaa.has_value());
        CHECK(*w.msaa == 1);
        REQUIRE(w.isDecorated.has_value());
        CHECK(*w.isDecorated == false);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 640);
        CHECK(w.pos->y == 100);
        CHECK(w.size.x == 640);
        CHECK(w.size.y == 480);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(20.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: Single", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(std::string(BASE_PATH) + "/config/single.json");

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
    const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
    CHECK(p.fov.left == -80.f / 2.f);
    CHECK(p.fov.right == 80.f / 2.f);
    CHECK(p.fov.up == 50.534015846724f / 2.f);
    CHECK(p.fov.down == -50.534015846724f / 2.f);
    REQUIRE(p.orientation.has_value());
    sgct::quat q = fromEuler(0.f, 0.f, 0.f);
    CHECK(p.orientation->x == q.x);
    CHECK(p.orientation->y == q.y);
    CHECK(p.orientation->z == q.z);
    CHECK(p.orientation->w == q.w);


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: Spherical Mirror 4 Meshes", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/spherical_mirror_4meshes.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.useFxaa.has_value());
    CHECK(*w.useFxaa == false);
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 1);
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "Spherical Projection (4 Meshes)");
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.pos.has_value());
    CHECK(w.pos->x == 0);
    CHECK(w.pos->y == 100);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<SphericalMirrorProjection>(v.projection));
    const SphericalMirrorProjection& p =
        std::get<SphericalMirrorProjection>(v.projection);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 2048);
    REQUIRE(p.tilt.has_value());
    CHECK(*p.tilt == 0.f);
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 1.f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);
    CHECK(p.mesh.bottom == "mesh/bottom_warp2.obj");
    CHECK(p.mesh.left == "mesh/left_warp2.obj");
    CHECK(p.mesh.right == "mesh/right_warp2.obj");
    CHECK(p.mesh.top == "mesh/top_warp2.obj");

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Spherical Mirror", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/spherical_mirror.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.useFxaa.has_value());
    CHECK(*w.useFxaa == false);
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 1);
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "Spherical Projection");
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.pos.has_value());
    CHECK(w.pos->x == 0);
    CHECK(w.pos->y == 100);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);
    REQUIRE(w.resolution.has_value());
    CHECK(w.resolution->x == 2048);
    CHECK(w.resolution->y == 2048);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);
    REQUIRE(v.correctionMeshTexture.has_value());
    REQUIRE(
        *v.correctionMeshTexture ==
        std::filesystem::absolute(
            std::string(BASE_PATH) + "/config/mesh/standard_16x9.data"
        ).string()
    );

    REQUIRE(std::holds_alternative<FisheyeProjection>(v.projection));
    const FisheyeProjection& p = std::get<FisheyeProjection>(v.projection);
    REQUIRE(p.fov.has_value());
    CHECK(*p.fov == 180.f);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 2048);
    REQUIRE(p.tilt.has_value());
    CHECK(*p.tilt == 0.f);
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 0.1f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Spout Output Cubemap", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/spout_output_cubemap.json"
    );

    CHECK(res.masterAddress == "localhost");
    
    REQUIRE(res.scene.has_value());
    REQUIRE(res.scene->offset.has_value());
    CHECK(res.scene->offset->x == 0.f);
    CHECK(res.scene->offset->y == 0.f);
    CHECK(res.scene->offset->z == 0.f);
    REQUIRE(res.scene->orientation.has_value());
    sgct::quat q = fromEuler(0.f, -90.f, 0.f);
    CHECK(res.scene->orientation->x == q.x);
    CHECK(res.scene->orientation->y == q.y);
    CHECK(res.scene->orientation->z == q.z);
    CHECK(res.scene->orientation->w == q.w);
    REQUIRE(res.scene->scale.has_value());
    CHECK(*res.scene->scale == 1.f);

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 4);
    CHECK(w.size.x == 1024);
    CHECK(w.size.y == 1024);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<SpoutOutputProjection>(v.projection));
    const SpoutOutputProjection& p = std::get<SpoutOutputProjection>(v.projection);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 1024);
    REQUIRE(p.mapping.has_value());
    CHECK(*p.mapping == SpoutOutputProjection::Mapping::Cubemap);
    CHECK(p.mappingSpoutName == "OS_CUBEMAP");
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 0.1f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);
    REQUIRE(p.channels.has_value());
    CHECK(p.channels->right == true);
    CHECK(p.channels->zLeft == true);
    CHECK(p.channels->bottom == true);
    CHECK(p.channels->top == true);
    CHECK(p.channels->left == true);
    CHECK(p.channels->zRight == true);
    REQUIRE(p.orientation.has_value());
    CHECK(p.orientation->x == 0.f);
    CHECK(p.orientation->y == 0.f);
    CHECK(p.orientation->z == 0.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Spout Output Equirectangular", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/spout_output_equirectangular.json"
    );

    CHECK(res.masterAddress == "localhost");
    
    REQUIRE(res.scene.has_value());
    REQUIRE(res.scene->offset.has_value());
    CHECK(res.scene->offset->x == 0.f);
    CHECK(res.scene->offset->y == 0.f);
    CHECK(res.scene->offset->z == 0.f);
    REQUIRE(res.scene->orientation.has_value());
    sgct::quat q = fromEuler(0.f, -90.f, 0.f);
    CHECK(res.scene->orientation->x == q.x);
    CHECK(res.scene->orientation->y == q.y);
    CHECK(res.scene->orientation->z == q.z);
    CHECK(res.scene->orientation->w == q.w);
    REQUIRE(res.scene->scale.has_value());
    CHECK(*res.scene->scale == 1.f);

    REQUIRE(res.settings.has_value());
    REQUIRE(res.settings->display.has_value());
    REQUIRE(res.settings->display->swapInterval.has_value());
    CHECK(*res.settings->display->swapInterval == 0);

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 4);
    CHECK(w.size.x == 1024);
    CHECK(w.size.y == 1024);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<SpoutOutputProjection>(v.projection));
    const SpoutOutputProjection& p = std::get<SpoutOutputProjection>(v.projection);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 1024);
    REQUIRE(p.mapping.has_value());
    CHECK(*p.mapping == SpoutOutputProjection::Mapping::Equirectangular);
    CHECK(p.mappingSpoutName == "OS_EQUIRECTANGULAR");
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 0.1f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);
    REQUIRE(p.channels.has_value());
    CHECK(p.channels->right == true);
    CHECK(p.channels->zLeft == true);
    CHECK(p.channels->bottom == true);
    CHECK(p.channels->top == true);
    CHECK(p.channels->left == true);
    CHECK(p.channels->zRight == true);
    REQUIRE(p.orientation.has_value());
    CHECK(p.orientation->x == 0.f);
    CHECK(p.orientation->y == 0.f);
    CHECK(p.orientation->z == 0.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Spout Output Fisheye", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/spout_output_fisheye.json"
    );

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.scene.has_value());
    REQUIRE(res.scene->offset.has_value());
    CHECK(res.scene->offset->x == 0.f);
    CHECK(res.scene->offset->y == 0.f);
    CHECK(res.scene->offset->z == 0.f);
    REQUIRE(res.scene->orientation.has_value());
    sgct::quat q = fromEuler(0.f, 0.f, 0.f);
    CHECK(res.scene->orientation->x == q.x);
    CHECK(res.scene->orientation->y == q.y);
    CHECK(res.scene->orientation->z == q.z);
    CHECK(res.scene->orientation->w == q.w);
    REQUIRE(res.scene->scale.has_value());
    CHECK(*res.scene->scale == 1.f);

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 4);
    CHECK(w.size.x == 1024);
    CHECK(w.size.y == 1024);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);

    REQUIRE(std::holds_alternative<SpoutOutputProjection>(v.projection));
    const SpoutOutputProjection& p = std::get<SpoutOutputProjection>(v.projection);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 1024);
    REQUIRE(p.mapping.has_value());
    CHECK(*p.mapping == SpoutOutputProjection::Mapping::Fisheye);
    CHECK(p.mappingSpoutName == "OS_FISHEYE");
    REQUIRE(p.background.has_value());
    CHECK(p.background->x == 0.1f);
    CHECK(p.background->y == 0.1f);
    CHECK(p.background->z == 0.1f);
    CHECK(p.background->w == 1.f);
    REQUIRE(p.channels.has_value());
    CHECK(p.channels->right == true);
    CHECK(p.channels->zLeft == true);
    CHECK(p.channels->bottom == false);
    CHECK(p.channels->top == true);
    CHECK(p.channels->left == true);
    CHECK(p.channels->zRight == false);
    REQUIRE(p.orientation.has_value());
    CHECK(p.orientation->x == 0.f);
    CHECK(p.orientation->y == 45.f);
    CHECK(p.orientation->z == 0.f);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Load: Two Nodes", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(
        std::string(BASE_PATH) + "/config/two_nodes.json"
    );

    CHECK(res.masterAddress == "127.0.0.1");

    REQUIRE(res.nodes.size() == 2);
    {
        const Node& n = res.nodes[0];
        CHECK(n.address == "127.0.0.1");
        CHECK(n.port == 20401);

        REQUIRE(n.windows.size() == 1);
        const Window& w = n.windows[0];
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 0);
        CHECK(w.pos->y == 300);
        CHECK(w.size.x == 640);
        CHECK(w.size.y == 360);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(-20.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }

    {
        const Node& n = res.nodes[1];
        CHECK(n.address == "127.0.0.2");
        CHECK(n.port == 20402);

        REQUIRE(n.windows.size() == 1);
        const Window& w = n.windows[0];
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 640);
        CHECK(w.pos->y == 300);
        CHECK(w.size.x == 640);
        CHECK(w.size.y == 360);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.position.has_value());
        CHECK(v.position->x == 0.f);
        CHECK(v.position->y == 0.f);
        REQUIRE(v.size.has_value());
        CHECK(v.size->x == 1.f);
        CHECK(v.size->y == 1.f);

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(20.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Load: TextureMappedProjection", "[parse]") {
    using namespace sgct::config;

    Cluster res = sgct::readConfig(std::string(BASE_PATH) +
        "/config/single_texturemapped.json");

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.isDecorated.has_value());
    CHECK(*w.isDecorated == false);
    CHECK(w.size.x == 768);
    CHECK(w.size.y == 810);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.position.has_value());
    CHECK(v.position->x == 0.f);
    CHECK(v.position->y == 0.f);
    REQUIRE(v.size.has_value());
    CHECK(v.size->x == 1.f);
    CHECK(v.size->y == 1.f);
    REQUIRE(v.correctionMeshTexture.has_value());
    std::string expectedPath = "mesh/surface1.pfm";
#ifdef WIN32
    expectedPath = "mesh\\surface1.pfm";
#endif
    std::string relativePath = *v.correctionMeshTexture;
    relativePath = relativePath.substr(relativePath.length() - expectedPath.length());
    CHECK(relativePath == expectedPath);

    REQUIRE(std::holds_alternative<TextureMappedProjection>(v.projection));
    const TextureMappedProjection& p = std::get<TextureMappedProjection>(v.projection);
    CHECK(p.fov.left == -77.82199f);
    CHECK(p.fov.right == 77.82199f);
    CHECK(p.fov.up == 78.43599f);
    CHECK(p.fov.down == -78.43599f);
    REQUIRE(p.orientation.has_value());

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.06f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}
