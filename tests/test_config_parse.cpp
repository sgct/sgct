/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
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

//
//  Configuration files that come with SGCT
//

TEST_CASE("Parse/SGCT: 3DTV", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20400,
      "windows": [
        {
          "fullscreen": true,
          "stereo": "side_by_side",
          "size": { "x": 1920, "y": 1080 },
          "viewports": [
            {
              "pos": { "x": 0, "y": 0 },
              "size": { "x": 1, "y": 1 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": -1.7156, "y": -0.965, "z": 0.0 },
                "upperleft": { "x": -1.7156, "y": 0.965, "z": 0.0 },
                "upperright": { "x": 1.7156, "y": 0.965, "z": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Kinect", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "Kinect0",
      "devices": [
        {
          "name": "Head",
          "sensors": [
            { "id": 0, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Neck",
          "sensors": [
            { "id": 1, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Torso",
          "sensors": [
            { "id": 2, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Waist",
          "sensors": [
            { "id": 3, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Collar",
          "sensors": [
            { "id": 4, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Shoulder",
          "sensors": [
            { "id": 5, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Elbow",
          "sensors": [
            { "id": 6, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Wrist",
          "sensors": [
            { "id": 7, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Hand",
          "sensors": [
            { "id": 8, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Fingertip",
          "sensors": [
            { "id": 9, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Collar",
          "sensors": [
            { "id": 10, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Shoulder",
          "sensors": [
            { "id": 11, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Elbow",
          "sensors": [
            { "id": 12, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Wrist",
          "sensors": [
            { "id": 13, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Hand",
          "sensors": [
            { "id": 14, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Fingertip",
          "sensors": [
            { "id": 15, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Hip",
          "sensors": [
            { "id": 16, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Knee",
          "sensors": [
            { "id": 17, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Ankle",
          "sensors": [
            { "id": 18, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Left Foot",
          "sensors": [
            { "id": 19, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Hip",
          "sensors": [
            { "id": 20, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Knee",
          "sensors": [
            { "id": 21, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Ankle",
          "sensors": [
            { "id": 22, "vrpnaddress": "Tracker0@localhost" }
          ]
        },
        {
          "name": "Right Foot",
          "sensors": [
            { "id": 23, "vrpnaddress": "Tracker0@localhost" }
          ]
        }
      ]
    }
  ],
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "msaa": 4,
          "fullscreen": false,
          "size": { "x": 960, "y": 540 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": -1.778, "y": -1.0, "z": 0.0 },
                "upperleft": { "x": -1.778, "y": 1.0, "z": 0.0 },
                "upperright": { "x": 1.778, "y": 1.0, "z": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 1.5 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Multi Window", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "msaa": 4,
          "fullscreen": false,
          "pos": { "x": 0, "y": 30 },
          "size": { "x": 960, "y": 540 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": -1.778, "y": 0.0, "z": 0.0 },
                "upperleft": { "x": -1.778, "y": 1.0, "z": 0.0 },
                "upperright": { "x": 0.0, "y": 1.0, "z": 0.0 }
              }
            }
          ]
        },
        {
          "msaa": 4,
          "fullscreen": false,
          "pos": { "x": 0, "y": 570 },
          "size": { "x": 480, "y": 540 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": -1.778, "y": -1, "z": 0.0 },
                "upperleft": { "x": -1.778, "y": 0.0, "z": 0.0 },
                "upperright": { "x": -0.889, "y": 0.0, "z": 0.0 }
              }
            }
          ]
        },
        {
          "msaa": 4,
          "fullscreen": false,
          "pos": { "x": 480, "y": 570 },
          "size": { "x": 480, "y": 270 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": -0.889, "y": -0.5, "z": 0.0 },
                "upperleft": { "x": -0.889, "y": 0.0, "z": 0.0 },
                "upperright": { "x": 0.0, "y": 0.0, "z": 0.0 }
              }
            }
          ]
        },
        {
          "msaa": 4,
          "fullscreen": false,
          "pos": { "x": 480, "y": 840 },
          "size": { "x": 480, "y": 270 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": -0.889, "y": -1.0, "z": 0.0 },
                "upperleft": { "x": -0.889, "y": -0.5, "z": 0.0 },
                "upperright": { "x": 0.0, "y": -0.5, "z": 0.0 }
              }
            }
          ]
        },
        {
          "msaa": 4,
          "border": false,
          "fullscreen": false,
          "pos": { "x": 1000, "y": 30 },
          "size": { "x": 960, "y": 1080 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "ProjectionPlane",
                "lowerleft": { "x": 0.0, "y": -1.0, "z": 0.0 },
                "upperleft": { "x": 0.0, "y": 1.0, "z": 0.0 },
                "upperright": { "x": 1.778, "y": 1.0, "z": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single Cylindrical", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "CylindricalProjection",
                "quality": "4k"
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single Equirectangular", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "EquirectangularProjection",
                "quality": "1k"
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single Fisheye FXAA", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fxaa": true,
          "fullscreen": false,
          "stereo": "none",
          "size": { "x": 512, "y": 512 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "fov": 180,
                "quality": "medium",
                "tilt": 0,
                "type": "FisheyeProjection",
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single Fisheye", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "stereo": "none",
          "size": { "x": 512, "y": 512 },
          "viewports": [
            {
              "type": "FisheyeProjection",
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "FisheyeProjection",
                "fov": 180.0,
                "quality": "medium",
                "tilt": 0.0,
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single SBS Stereo", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "stereo": "side_by_side",
          "pos": { "x": 200, "y": 300 },
          "size": { "x": 1280, "y": 360 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single Two Win 3D", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "id": 1,
          "name": "3D Rendering",
          "msaa": 1,
          "draw2d": false,
          "fullscreen": false,
          "pos": { "x": 10, "y": 100 },
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        },
        {
          "id": 0,
          "name": "2D Overlay",
          "msaa": 1,
          "draw3d": false,
          "blitwindowid": 1,
          "fullscreen": false,
          "pos": { "x": 640, "y": 100 },
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single Two Win", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "msaa": 1,
          "border": true,
          "fullscreen": false,
          "pos": { "x": 10, "y": 100 },
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": -20.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        },
        {
          "msaa": 1,
          "border": false,
          "fullscreen": false,
          "pos": { "x": 640, "y": 100 },
          "size": { "x": 640, "y": 480 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 20.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Single", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0, "y": 0 },
              "size": { "x": 1, "y": 1 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0, "pitch": 0, "roll": 0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0, "y": 0, "z": 4 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Spherical Mirror 4 Meshes", "[parse]") {
    constexpr std::string_view Source = R"a(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "fxaa": false,
          "msaa": 1,
          "name": "Spherical Projection (4 Meshes)",
          "stereo": "none",
          "pos": { "x": 0, "y": 100 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "SphericalMirrorProjection",
                "quality": "2k",
                "tilt": 0.0,
                "background": { "r": 1.0, "g": 0.1, "b": 0.1, "a": 1.0 },
                "geometry": {
                  "bottom": "mesh/bottom_warp2.obj",
                  "left": "mesh/left_warp2.obj",
                  "right": "mesh/right_warp2.obj",
                  "top": "mesh/top_warp2.obj"
                }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})a";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Spherical Mirror", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "fxaa": false,
          "msaa": 1,
          "name": "Spherical Projection",
          "stereo": "none",
          "pos": { "x": 0, "y": 100 },
          "size": { "x": 1280, "y": 720 },
          "res": { "x": 2048, "y": 2048 },
          "viewports": [
            {
              "mesh": "mesh/standard_16x9.data",
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "FisheyeProjection",
                "fov": 180.0,
                "quality": "2k",
                "tilt": 0.0,
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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
        std::filesystem::absolute("mesh/standard_16x9.data").string()
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

TEST_CASE("Parse/SGCT: Spout Output Cubemap", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "orientation": { "yaw": 0.0, "pitch": -90.0, "roll": 0.0 },
    "scale": 1.0
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "stereo": "none",
          "msaa": 4,
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "SpoutOutputProjection",
                "quality": "1k",
                "mapping": "cubemap",
                "mappingspoutname": "OS_CUBEMAP",
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 },
                "channels": {
                  "right": true,
                  "zleft": true,
                  "bottom": true,
                  "top": true,
                  "left": true,
                  "zright": true
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Spout Output Equirectangular", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "orientation": { "yaw": 0.0, "pitch": -90.0, "roll": 0.0 },
    "scale": 1.0
  },
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "stereo": "none",
          "msaa": 4,
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "SpoutOutputProjection",
                "quality": "1k",
                "mapping": "equirectangular",
                "mappingspoutname": "OS_EQUIRECTANGULAR",
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 },
                "channels": {
                  "right": true,
                  "zleft": true,
                  "bottom": true,
                  "top": true,
                  "left": true,
                  "zright": true
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Spout Output Fisheye", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "scene": {
    "offset": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 },
    "scale": 1.0
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "stereo": "none",
          "msaa": 4,
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "SpoutOutputProjection",
                "quality": "1k",
                "mapping": "fisheye",
                "mappingspoutname": "OS_FISHEYE",
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 },
                "channels": {
                  "right": true,
                  "zleft": true,
                  "bottom": false,
                  "top": true,
                  "left": true,
                  "zright": false
                },
                "orientation": { "yaw": 45.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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

TEST_CASE("Parse/SGCT: Two Nodes", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "127.0.0.1",
  "nodes": [
    {
      "address": "127.0.0.1",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "pos": { "x": 0, "y": 300 },
          "size": { "x": 640, "y": 360 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": -20.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    },
    {
      "address": "127.0.0.2",
      "port": 20402,
      "windows": [
        {
          "fullscreen": false,
          "pos": { "x": 640, "y": 300 },
          "size": { "x": 640, "y": 360 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 20.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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


//
//  Configuration files used in other projects
//

TEST_CASE("Parse/OpenSpace: Equirectangular GUI", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "name": "OpenSpace",
          "fullscreen": false,
          "draw2d": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "EquirectangularProjection",
                "quality": "1k"
              }
            }
          ]
        },
        {
          "name": "GUI",
          "tags": [ "GUI" ],
          "fullscreen": false,
          "draw3d": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.settings.has_value());
    REQUIRE(res.settings->display.has_value());
    REQUIRE(res.settings->display->swapInterval.has_value());
    CHECK(*res.settings->display->swapInterval == 0);

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "OpenSpace");
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.draw2D.has_value());
        CHECK(*w.draw2D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 50);
        CHECK(w.pos->y == 50);
        CHECK(w.size.x == 1280);
        CHECK(w.size.y == 720);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.isTracked.has_value());
        CHECK(*v.isTracked == true);
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
    }
    {
        const Window& w = n.windows[1];
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "GUI");
        REQUIRE(w.tags.size() == 1);
        CHECK(w.tags[0] == "GUI");
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.draw3D.has_value());
        CHECK(*w.draw3D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 50);
        CHECK(w.pos->y == 50);
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

        CHECK(std::holds_alternative<NoProjection>(v.projection));
    }

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Fullscreen 1080", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": true,
          "name": "OpenSpace",
          "msaa": 4,
          "stereo": "none",
          "pos": { "x": 0, "y": 0 },
          "size": { "x": 1920, "y": 1080 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

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
    CHECK(*w.isFullScreen == true);
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "OpenSpace");
    REQUIRE(w.msaa.has_value());
    CHECK(*w.msaa == 4);
    REQUIRE(w.stereo.has_value());
    CHECK(w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.pos.has_value());
    CHECK(w.pos->x == 0);
    CHECK(w.pos->y == 0);
    CHECK(w.size.x == 1920);
    CHECK(w.size.y == 1080);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.isTracked.has_value());
    CHECK(*v.isTracked == true);
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
    sgct::quat q = fromEuler(0.f, 0.f, 0.f);
    CHECK(p.orientation->x == q.x);
    CHECK(p.orientation->y == q.y);
    CHECK(p.orientation->z == q.z);
    CHECK(p.orientation->w == q.w);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: GUI Projector", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": true,
          "monitor": 1,
          "name": "OpenSpace",
          "draw2d": false,
          "stereo": "none",
          "pos": { "x": 0, "y": 0 },
          "size": { "x": 1920, "y": 1080 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        },
        {
          "fullscreen": false,
          "border": false,
          "name": "GUI",
          "tags": [ "GUI" ],
          "draw3d": false,
          "stereo": "none",
          "pos": { "x": 0, "y": 0 },
          "size": { "x": 1920, "y": 1080 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(res.settings.has_value());
    REQUIRE(res.settings->display.has_value());
    REQUIRE(res.settings->display->swapInterval.has_value());
    CHECK(*res.settings->display->swapInterval == 0);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == true);
        REQUIRE(w.monitor.has_value());
        CHECK(*w.monitor == 1);
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "OpenSpace");
        REQUIRE(w.draw2D.has_value());
        CHECK(*w.draw2D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 0);
        CHECK(w.pos->y == 0);
        CHECK(w.size.x == 1920);
        CHECK(w.size.y == 1080);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.isTracked.has_value());
        CHECK(*v.isTracked == true);
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
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }
    {
        const Window& w = n.windows[1];
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.isDecorated.has_value());
        CHECK(*w.isDecorated == false);
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "GUI");
        REQUIRE(w.tags.size() == 1);
        CHECK(w.tags[0] == "GUI");
        REQUIRE(w.draw3D.has_value());
        CHECK(*w.draw3D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 0);
        CHECK(w.pos->y == 0);
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

        CHECK(std::holds_alternative<NoProjection>(v.projection));
    }


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Single Fisheye GUI", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "name": "OpenSpace",
          "fullscreen": false,
          "draw2d": false,
          "stereo": "none",
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "FisheyeProjection",
                "fov": 180.0,
                "quality": "1k",
                "tilt": 27.0,
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        },
        {
          "name": "GUI",
          "tags": [ "GUI" ],
          "fullscreen": false,
          "draw3d": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "OpenSpace");
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.draw2D.has_value());
        CHECK(*w.draw2D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
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

        REQUIRE(std::holds_alternative<FisheyeProjection>(v.projection));
        const FisheyeProjection& p = std::get<FisheyeProjection>(v.projection);
        REQUIRE(p.fov.has_value());
        CHECK(*p.fov == 180.f);
        REQUIRE(p.quality.has_value());
        CHECK(*p.quality == 1024);
        REQUIRE(p.tilt.has_value());
        CHECK(*p.tilt == 27.f);
        REQUIRE(p.background.has_value());
        CHECK(p.background->x == 0.1f);
        CHECK(p.background->y == 0.1f);
        CHECK(p.background->z == 0.1f);
        CHECK(p.background->w == 1.f);
    }
    {
        const Window& w = n.windows[1];
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "GUI");
        REQUIRE(w.tags.size() == 1);
        CHECK(w.tags[0] == "GUI");
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.draw3D.has_value());
        CHECK(*w.draw3D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 50);
        CHECK(w.pos->y == 50);
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

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
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
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Single Fisheye", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "name": "OpenSpace",
          "fullscreen": false,
          "stereo": "none",
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "FisheyeProjection",
                "fov": 180.0,
                "quality": "1k",
                "tilt": 27.0,
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "OpenSpace");
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(w.stereo == Window::StereoMode::NoStereo);
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

    REQUIRE(std::holds_alternative<FisheyeProjection>(v.projection));
    const FisheyeProjection& p = std::get<FisheyeProjection>(v.projection);
    REQUIRE(p.fov.has_value());
    CHECK(*p.fov == 180.f);
    REQUIRE(p.quality.has_value());
    CHECK(*p.quality == 1024);
    REQUIRE(p.tilt.has_value());
    CHECK(*p.tilt == 27.f);
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

TEST_CASE("Parse/OpenSpace: Single GUI", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "name": "OpenSpace",
          "fullscreen": false,
          "draw2d": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        },
        {
          "name": "GUI",
          "tags": [ "GUI" ],
          "fullscreen": false,
          "draw3d": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.settings.has_value());
    REQUIRE(res.settings->display.has_value());
    REQUIRE(res.settings->display->swapInterval.has_value());
    CHECK(*res.settings->display->swapInterval == 0);

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "OpenSpace");
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.draw2D.has_value());
        CHECK(*w.draw2D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 50);
        CHECK(w.pos->y == 50);
        CHECK(w.size.x == 1280);
        CHECK(w.size.y == 720);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.isTracked.has_value());
        CHECK(*v.isTracked == true);
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
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }
    {
        const Window& w = n.windows[1];
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "GUI");
        REQUIRE(w.tags.size() == 1);
        CHECK(w.tags[0] == "GUI");
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.draw3D.has_value());
        CHECK(*w.draw3D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 50);
        CHECK(w.pos->y == 50);
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
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Single SBS Stereo", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "name": "OpenSpace",
          "fullscreen": false,
          "stereo": "side_by_side",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

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
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "OpenSpace");
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(w.stereo == Window::StereoMode::SideBySide);
    REQUIRE(w.pos.has_value());
    CHECK(w.pos->x == 50);
    CHECK(w.pos->y == 50);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.isTracked.has_value());
    CHECK(*v.isTracked == true);
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
    sgct::quat q = fromEuler(0.f, 0.f, 0.f);
    CHECK(p.orientation->x == q.x);
    CHECK(p.orientation->y == q.y);
    CHECK(p.orientation->z == q.z);
    CHECK(p.orientation->w == q.w);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Single Two Win", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "127.0.0.1",
  "nodes": [
    {
      "address": "127.0.0.1",
      "port": 20401,
      "windows": [
        {
          "border": true,
          "fullscreen": false,
          "pos": { "x": 10, "y": 100 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        },
        {
          "border": true,
          "fullscreen": false,
          "pos": { "x": 340, "y": 100 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "127.0.0.1");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "127.0.0.1");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.isDecorated.has_value());
        CHECK(*w.isDecorated == true);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 10);
        CHECK(w.pos->y == 100);
        CHECK(w.size.x == 1280);
        CHECK(w.size.y == 720);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.isTracked.has_value());
        CHECK(*v.isTracked == true);
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
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }
    {
        const Window& w = n.windows[1];
        REQUIRE(w.isDecorated.has_value());
        CHECK(*w.isDecorated == true);
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 340);
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

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}

TEST_CASE("Parse/OpenSpace: Single", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "settings": {
    "display": {
      "swapinterval": 0
    }
  },
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "name": "OpenSpace",
          "fullscreen": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.settings.has_value());
    REQUIRE(res.settings->display.has_value());
    REQUIRE(*res.settings->display->swapInterval == 0);
        
    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "OpenSpace");
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
    REQUIRE(w.pos.has_value());
    CHECK(w.pos->x == 50);
    CHECK(w.pos->y == 50);
    CHECK(w.size.x == 1280);
    CHECK(w.size.y == 720);

    REQUIRE(w.viewports.size() == 1);
    const Viewport& v = w.viewports[0];
    REQUIRE(v.isTracked.has_value());
    CHECK(*v.isTracked == true);
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
    sgct::quat q = fromEuler(0.f, 0.f, 0.f);
    CHECK(p.orientation->x == q.x);
    CHECK(p.orientation->y == q.y);
    CHECK(p.orientation->z == q.z);
    CHECK(p.orientation->w == q.w);

    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Spherical Mirror GUI", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "127.0.0.1",
  "nodes": [
    {
      "address": "127.0.0.1",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "name": "Spherical Projection",
          "stereo": "none",
          "draw2d": false,
          "pos": { "x": 0, "y": 100 },
          "size": { "x": 1280, "y": 720 },
          "res": { "x": 2048, "y": 2048 },
          "viewports": [
            {
              "mesh": "mesh/standard_16x9.data",
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "FisheyeProjection",
                "fov": 180.0,
                "quality": "2k",
                "tilt": 30.0,
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        },
        {
          "fullscreen": false,
          "name": "GUI",
          "tags": [ "GUI" ],
          "draw3d": false,
          "stereo": "none",
          "pos": { "x": 50, "y": 50 },
          "size": { "x": 1280, "y": 720 },
          "res": { "x": 2048, "y": 2048 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "127.0.0.1");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "127.0.0.1");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 2);
    {
        const Window& w = n.windows[0];
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "Spherical Projection");
        REQUIRE(w.stereo.has_value());
        CHECK(*w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.draw2D.has_value());
        CHECK(*w.draw2D == false);
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
        REQUIRE(v.correctionMeshTexture.has_value());
        CHECK(
            *v.correctionMeshTexture ==
            std::filesystem::absolute("mesh/standard_16x9.data").string()
        );
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
        CHECK(*p.quality == 2048);
        REQUIRE(p.tilt.has_value());
        CHECK(*p.tilt == 30.f);
        REQUIRE(p.background.has_value());
        CHECK(p.background->x == 0.1f);
        CHECK(p.background->y == 0.1f);
        CHECK(p.background->z == 0.1f);
        CHECK(p.background->w == 1.f);
    }
    {
        const Window& w = n.windows[1];
        REQUIRE(w.isFullScreen.has_value());
        CHECK(*w.isFullScreen == false);
        REQUIRE(w.name.has_value());
        CHECK(*w.name == "GUI");
        REQUIRE(w.tags.size() == 1);
        CHECK(w.tags[0] == "GUI");
        REQUIRE(w.draw3D.has_value());
        CHECK(*w.draw3D == false);
        REQUIRE(w.stereo.has_value());
        CHECK(*w.stereo == Window::StereoMode::NoStereo);
        REQUIRE(w.pos.has_value());
        CHECK(w.pos->x == 50);
        CHECK(w.pos->y == 50);
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

        REQUIRE(std::holds_alternative<PlanarProjection>(v.projection));
        const PlanarProjection& p = std::get<PlanarProjection>(v.projection);
        CHECK(p.fov.left == -80.f / 2.f);
        CHECK(p.fov.right == 80.f / 2.f);
        CHECK(p.fov.down == -50.534015846724f / 2.f);
        CHECK(p.fov.up == 50.534015846724f / 2.f);
        REQUIRE(p.orientation.has_value());
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
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
    CHECK(u.position->z == 0.f);
}

TEST_CASE("Parse/OpenSpace: Spherical Mirror", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "127.0.0.1",
  "nodes": [
    {
      "address": "127.0.0.1",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "name": "Spherical Projection",
          "stereo": "none",
          "pos": { "x": 0, "y": 100 },
          "size": { "x": 1280, "y": 720 },
          "res": { "x": 2048, "y": 2048 },
          "viewports": [
            {
              "mesh": "mesh/standard_16x9.data",
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "FisheyeProjection",
                "fov": 180.0,
                "quality": "2k",
                "tilt": 30.0,
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "127.0.0.1");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "127.0.0.1");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
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
    REQUIRE(v.correctionMeshTexture.has_value());
    CHECK(
        *v.correctionMeshTexture ==
        std::filesystem::absolute("mesh/standard_16x9.data").string()
    );
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
    CHECK(*p.quality == 2048);
    REQUIRE(p.tilt.has_value());
    CHECK(*p.tilt == 30.f);
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

TEST_CASE("Parse/OpenSpace: Spout Output", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "nodes": [
    {
      "address": "localhost",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "name": "OpenSpace",
          "stereo": "none",
          "size": { "x": 1024, "y": 1024 },
          "viewports": [
            {
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "SpoutOutputProjection",
                "quality": "1k",
                "mappingspoutname": "OpenSpace",
                "background": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.06,
      "pos": { "x": 0.0, "y": 0.0, "z": 0.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

    CHECK(res.masterAddress == "localhost");

    REQUIRE(res.nodes.size() == 1);
    const Node& n = res.nodes[0];
    CHECK(n.address == "localhost");
    CHECK(n.port == 20401);

    REQUIRE(n.windows.size() == 1);
    const Window& w = n.windows[0];
    REQUIRE(w.isFullScreen.has_value());
    CHECK(*w.isFullScreen == false);
    REQUIRE(w.name.has_value());
    CHECK(*w.name == "OpenSpace");
    REQUIRE(w.stereo.has_value());
    CHECK(*w.stereo == Window::StereoMode::NoStereo);
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
    CHECK(p.mappingSpoutName == "OpenSpace");
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

TEST_CASE("Parse/OpenSpace: Two Nodes", "[parse]") {
    constexpr std::string_view Source = R"(
{
  "version": 1,
  "masteraddress": "127.0.0.1",
  "nodes": [
    {
      "address": "127.0.0.1",
      "port": 20401,
      "windows": [
        {
          "fullscreen": false,
          "pos": { "x": 0, "y": 300 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    },
    {
      "address": "127.0.0.2",
      "port": 20402,
      "windows": [
        {
          "fullscreen": false,
          "pos": { "x": 640, "y": 300 },
          "size": { "x": 1280, "y": 720 },
          "viewports": [
            {
              "tracked": true,
              "pos": { "x": 0.0, "y": 0.0 },
              "size": { "x": 1.0, "y": 1.0 },
              "projection": {
                "type": "PlanarProjection",
                "fov": {
                  "hfov": 80.0,
                  "vfov": 50.534015846724
                },
                "orientation": { "yaw": 0.0, "pitch": 0.0, "roll": 0.0 }
              }
            }
          ]
        }
      ]
    }
  ],
  "users": [
    {
      "eyeseparation": 0.065,
      "pos": { "x": 0.0, "y": 0.0, "z": 4.0 }
    }
  ]
})";

    using namespace sgct::config;

    Cluster res = sgct::readJsonConfig(Source);

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
        CHECK(w.size.x == 1280);
        CHECK(w.size.y == 720);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.isTracked.has_value());
        CHECK(*v.isTracked == true);
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
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
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
        CHECK(w.size.x == 1280);
        CHECK(w.size.y == 720);

        REQUIRE(w.viewports.size() == 1);
        const Viewport& v = w.viewports[0];
        REQUIRE(v.isTracked.has_value());
        CHECK(*v.isTracked == true);
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
        sgct::quat q = fromEuler(0.f, 0.f, 0.f);
        CHECK(p.orientation->x == q.x);
        CHECK(p.orientation->y == q.y);
        CHECK(p.orientation->z == q.z);
        CHECK(p.orientation->w == q.w);
    }


    REQUIRE(res.users.size() == 1);
    const User& u = res.users[0];
    REQUIRE(u.eyeSeparation.has_value());
    CHECK(*u.eyeSeparation == 0.065f);
    REQUIRE(u.position.has_value());
    CHECK(u.position->x == 0.f);
    CHECK(u.position->y == 0.f);
    CHECK(u.position->z == 4.f);
}
