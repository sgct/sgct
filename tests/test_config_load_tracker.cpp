/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/config.h>
#include <sgct/math.h>

using namespace sgct;
using namespace sgct::config;

TEST_CASE("Load: Tracker/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": []
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .trackers = {}
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Tracker/Name", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc"
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc"
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "def"
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "def"
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Devices/Minimal", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": []
    }
  ]
}
)";

    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .trackers = {
            Tracker {
                .name = "abc",
                .devices = {}
            }
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}

TEST_CASE("Load: Tracker/Devices/Name", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "abc"
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "abc"
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def"
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def"
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "abc"
        },
        {
          "name": "def"
        },
        {
          "name": "ghi"
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "abc"
                        },
                        Tracker::Device {
                            .name = "def"
                        },
                        Tracker::Device {
                            .name = "ghi"
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Devices/Sensors", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "sensors": []
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .sensors = {}
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "sensors": [
            {
              "vrpnaddress": "abc",
              "id": 1
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .sensors = {
                                Tracker::Device::Sensor {
                                    .vrpnAddress = "abc",
                                    .identifier = 1
                                }
                            }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "sensors": [
            {
              "vrpnaddress": "def",
              "id": 2
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .sensors = {
                                Tracker::Device::Sensor {
                                    .vrpnAddress = "def",
                                    .identifier = 2
                                }
                            }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Devices/Buttons", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "buttons": []
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .buttons = {}
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "buttons": [
            {
              "vrpnaddress": "abc",
              "count": 1
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .buttons = {
                                Tracker::Device::Button {
                                    .vrpnAddress = "abc",
                                    .count = 1
                                }
                            }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "buttons": [
            {
              "vrpnaddress": "def",
              "count": 2
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .buttons = {
                                Tracker::Device::Button {
                                    .vrpnAddress = "def",
                                    .count = 2
                                }
                            }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Devices/Axes", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "axes": []
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .axes = {}
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "axes": [
            {
              "vrpnaddress": "abc",
              "count": 1
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .axes = {
                                Tracker::Device::Axis {
                                    .vrpnAddress = "abc",
                                    .count = 1
                                }
                            }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "axes": [
            {
              "vrpnaddress": "def",
              "count": 2
            }
          ]
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .axes = {
                                Tracker::Device::Axis {
                                    .vrpnAddress = "def",
                                    .count = 2
                                }
                            }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Devices/Offset", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "offset": { "x": 1.0, "y": 2.0, "z": 3.0 }
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .offset = vec3 { 1.f, 2.f, 3.f }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "offset": { "x": 4.0, "y": 5.0, "z": 6.0 }
        }
      ]
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .offset = vec3 { 4.f, 5.f, 6.f }
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Devices/Transformation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "matrix": [
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
            9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
          ]
        }
      ]
    }
  ]
}
)";

        mat4 m;
        m.values[0] = 1.f;
        m.values[1] = 2.f;
        m.values[2] = 3.f;
        m.values[3] = 4.f;
        m.values[4] = 5.f;
        m.values[5] = 6.f;
        m.values[6] = 7.f;
        m.values[7] = 8.f;
        m.values[8] = 9.f;
        m.values[9] = 10.f;
        m.values[10] = 11.f;
        m.values[11] = 12.f;
        m.values[12] = 13.f;
        m.values[13] = 14.f;
        m.values[14] = 15.f;
        m.values[15] = 16.f;
        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .transformation = m
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "matrix": [
            17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
            25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0
          ]
        }
      ]
    }
  ]
}
)";

        mat4 m;
        m.values[0] = 17.f;
        m.values[1] = 18.f;
        m.values[2] = 19.f;
        m.values[3] = 20.f;
        m.values[4] = 21.f;
        m.values[5] = 22.f;
        m.values[6] = 23.f;
        m.values[7] = 24.f;
        m.values[8] = 25.f;
        m.values[9] = 26.f;
        m.values[10] = 27.f;
        m.values[11] = 28.f;
        m.values[12] = 29.f;
        m.values[13] = 30.f;
        m.values[14] = 31.f;
        m.values[15] = 32.f;
        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .devices = {
                        Tracker::Device {
                            .name = "def",
                            .transformation = m
                        }
                    }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Offset", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "offset": { "x": 1.0, "y": 2.0, "z": 3.0 }
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .offset = vec3{ 1.f, 2.f, 3.f }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "offset": { "x": 4.0, "y": 5.0, "z": 6.0 }
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .offset = vec3{ 4.f, 5.f, 6.f }
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Scale", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "scale": 1.0
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .scale = 1.f
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "scale": 2.0
    }
  ]
}
)";

        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .scale = 2.f
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Transformation", "[parse]") {
    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "matrix": [
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
      ]
    }
  ]
}
)";

        mat4 m;
        m.values[0] = 1.f;
        m.values[1] = 2.f;
        m.values[2] = 3.f;
        m.values[3] = 4.f;
        m.values[4] = 5.f;
        m.values[5] = 6.f;
        m.values[6] = 7.f;
        m.values[7] = 8.f;
        m.values[8] = 9.f;
        m.values[9] = 10.f;
        m.values[10] = 11.f;
        m.values[11] = 12.f;
        m.values[12] = 13.f;
        m.values[13] = 14.f;
        m.values[14] = 15.f;
        m.values[15] = 16.f;
        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .transformation = m
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }

    {
        constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "matrix": [
        17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
        25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0
      ]
    }
  ]
}
)";

        mat4 m;
        m.values[0] = 17.f;
        m.values[1] = 18.f;
        m.values[2] = 19.f;
        m.values[3] = 20.f;
        m.values[4] = 21.f;
        m.values[5] = 22.f;
        m.values[6] = 23.f;
        m.values[7] = 24.f;
        m.values[8] = 25.f;
        m.values[9] = 26.f;
        m.values[10] = 27.f;
        m.values[11] = 28.f;
        m.values[12] = 29.f;
        m.values[13] = 30.f;
        m.values[14] = 31.f;
        m.values[15] = 32.f;
        const Cluster Object = {
            .success = true,
            .masterAddress = "localhost",
            .trackers = {
                Tracker {
                    .name = "abc",
                    .transformation = m
                }
            }
        };

        Cluster res = sgct::readJsonConfig(String);
        CHECK(res == Object);

        const std::string str = serializeConfig(Object);
        const config::Cluster output = readJsonConfig(str);
        CHECK(output == Object);
    }
}

TEST_CASE("Load: Tracker/Full", "[parse]") {
    constexpr std::string_view String = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": [
    {
      "name": "abc",
      "devices": [
        {
          "name": "def",
          "sensors": [
            {
              "vrpnaddress": "ghi",
              "id": 2
            }
          ],
          "buttons": [
            {
              "vrpnaddress": "jkl",
              "count": 3
            }
          ],
          "axes": [
            {
              "vrpnaddress": "mno",
              "count": 4
            }
          ],
          "offset": { "x": 5.0, "y": 6.0, "z": 7.0 },
          "matrix": [
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
            9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
          ]
        }
      ],
      "offset": { "x": 8.0, "y": 9.0, "z": 10.0 },
      "scale": 11.0,
      "matrix": [
        17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
        25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0
      ]
    }
  ]
}
)";

    mat4 m;
    m.values[0] = 1.f;
    m.values[1] = 2.f;
    m.values[2] = 3.f;
    m.values[3] = 4.f;
    m.values[4] = 5.f;
    m.values[5] = 6.f;
    m.values[6] = 7.f;
    m.values[7] = 8.f;
    m.values[8] = 9.f;
    m.values[9] = 10.f;
    m.values[10] = 11.f;
    m.values[11] = 12.f;
    m.values[12] = 13.f;
    m.values[13] = 14.f;
    m.values[14] = 15.f;
    m.values[15] = 16.f;
    mat4 m2;
    m2.values[0] = 17.f;
    m2.values[1] = 18.f;
    m2.values[2] = 19.f;
    m2.values[3] = 20.f;
    m2.values[4] = 21.f;
    m2.values[5] = 22.f;
    m2.values[6] = 23.f;
    m2.values[7] = 24.f;
    m2.values[8] = 25.f;
    m2.values[9] = 26.f;
    m2.values[10] = 27.f;
    m2.values[11] = 28.f;
    m2.values[12] = 29.f;
    m2.values[13] = 30.f;
    m2.values[14] = 31.f;
    m2.values[15] = 32.f;
    const Cluster Object = {
        .success = true,
        .masterAddress = "localhost",
        .trackers = {
            Tracker {
                .name = "abc",
                .devices = {
                    Tracker::Device {
                        .name = "def",
                        .sensors = {
                            Tracker::Device::Sensor {
                                .vrpnAddress = "ghi",
                                .identifier = 2
                            }
                        },
                        .buttons = {
                            Tracker::Device::Button {
                                .vrpnAddress = "jkl",
                                .count = 3
                            }
                        },
                        .axes = {
                            Tracker::Device::Axis {
                                .vrpnAddress = "mno",
                                .count = 4
                            }
                        },
                        .offset = vec3 { 5.f, 6.f, 7.f },
                        .transformation = m

                    }
                },
                .offset = vec3{ 8.f, 9.f, 10.f },
                .scale = 11.f,
                .transformation = m2

            }
        }
    };

    Cluster res = sgct::readJsonConfig(String);
    CHECK(res == Object);

    const std::string str = serializeConfig(Object);
    const config::Cluster output = readJsonConfig(str);
    CHECK(output == Object);
}
