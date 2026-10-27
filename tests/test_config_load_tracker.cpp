/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <sgct/config.h>
#include <sgct/glm.h>
#include "schema.h"

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
                            .offset = glm::vec3(1.f, 2.f, 3.f)
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
                            .offset = glm::vec3(4.f, 5.f, 6.f)
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

        glm::mat4 m;
        glm::value_ptr(m)[0] = 1.f;
        glm::value_ptr(m)[1] = 2.f;
        glm::value_ptr(m)[2] = 3.f;
        glm::value_ptr(m)[3] = 4.f;
        glm::value_ptr(m)[4] = 5.f;
        glm::value_ptr(m)[5] = 6.f;
        glm::value_ptr(m)[6] = 7.f;
        glm::value_ptr(m)[7] = 8.f;
        glm::value_ptr(m)[8] = 9.f;
        glm::value_ptr(m)[9] = 10.f;
        glm::value_ptr(m)[10] = 11.f;
        glm::value_ptr(m)[11] = 12.f;
        glm::value_ptr(m)[12] = 13.f;
        glm::value_ptr(m)[13] = 14.f;
        glm::value_ptr(m)[14] = 15.f;
        glm::value_ptr(m)[15] = 16.f;
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

        glm::mat4 m;
        glm::value_ptr(m)[0] = 17.f;
        glm::value_ptr(m)[1] = 18.f;
        glm::value_ptr(m)[2] = 19.f;
        glm::value_ptr(m)[3] = 20.f;
        glm::value_ptr(m)[4] = 21.f;
        glm::value_ptr(m)[5] = 22.f;
        glm::value_ptr(m)[6] = 23.f;
        glm::value_ptr(m)[7] = 24.f;
        glm::value_ptr(m)[8] = 25.f;
        glm::value_ptr(m)[9] = 26.f;
        glm::value_ptr(m)[10] = 27.f;
        glm::value_ptr(m)[11] = 28.f;
        glm::value_ptr(m)[12] = 29.f;
        glm::value_ptr(m)[13] = 30.f;
        glm::value_ptr(m)[14] = 31.f;
        glm::value_ptr(m)[15] = 32.f;
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
                    .offset = glm::vec3(1.f, 2.f, 3.f)
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
                    .offset = glm::vec3(4.f, 5.f, 6.f)
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

        glm::mat4 m;
        glm::value_ptr(m)[0] = 1.f;
        glm::value_ptr(m)[1] = 2.f;
        glm::value_ptr(m)[2] = 3.f;
        glm::value_ptr(m)[3] = 4.f;
        glm::value_ptr(m)[4] = 5.f;
        glm::value_ptr(m)[5] = 6.f;
        glm::value_ptr(m)[6] = 7.f;
        glm::value_ptr(m)[7] = 8.f;
        glm::value_ptr(m)[8] = 9.f;
        glm::value_ptr(m)[9] = 10.f;
        glm::value_ptr(m)[10] = 11.f;
        glm::value_ptr(m)[11] = 12.f;
        glm::value_ptr(m)[12] = 13.f;
        glm::value_ptr(m)[13] = 14.f;
        glm::value_ptr(m)[14] = 15.f;
        glm::value_ptr(m)[15] = 16.f;
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

        glm::mat4 m;
        glm::value_ptr(m)[0] = 17.f;
        glm::value_ptr(m)[1] = 18.f;
        glm::value_ptr(m)[2] = 19.f;
        glm::value_ptr(m)[3] = 20.f;
        glm::value_ptr(m)[4] = 21.f;
        glm::value_ptr(m)[5] = 22.f;
        glm::value_ptr(m)[6] = 23.f;
        glm::value_ptr(m)[7] = 24.f;
        glm::value_ptr(m)[8] = 25.f;
        glm::value_ptr(m)[9] = 26.f;
        glm::value_ptr(m)[10] = 27.f;
        glm::value_ptr(m)[11] = 28.f;
        glm::value_ptr(m)[12] = 29.f;
        glm::value_ptr(m)[13] = 30.f;
        glm::value_ptr(m)[14] = 31.f;
        glm::value_ptr(m)[15] = 32.f;
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

    glm::mat4 m;
    glm::value_ptr(m)[0] = 1.f;
    glm::value_ptr(m)[1] = 2.f;
    glm::value_ptr(m)[2] = 3.f;
    glm::value_ptr(m)[3] = 4.f;
    glm::value_ptr(m)[4] = 5.f;
    glm::value_ptr(m)[5] = 6.f;
    glm::value_ptr(m)[6] = 7.f;
    glm::value_ptr(m)[7] = 8.f;
    glm::value_ptr(m)[8] = 9.f;
    glm::value_ptr(m)[9] = 10.f;
    glm::value_ptr(m)[10] = 11.f;
    glm::value_ptr(m)[11] = 12.f;
    glm::value_ptr(m)[12] = 13.f;
    glm::value_ptr(m)[13] = 14.f;
    glm::value_ptr(m)[14] = 15.f;
    glm::value_ptr(m)[15] = 16.f;
    glm::mat4 m2;
    glm::value_ptr(m2)[0] = 17.f;
    glm::value_ptr(m2)[1] = 18.f;
    glm::value_ptr(m2)[2] = 19.f;
    glm::value_ptr(m2)[3] = 20.f;
    glm::value_ptr(m2)[4] = 21.f;
    glm::value_ptr(m2)[5] = 22.f;
    glm::value_ptr(m2)[6] = 23.f;
    glm::value_ptr(m2)[7] = 24.f;
    glm::value_ptr(m2)[8] = 25.f;
    glm::value_ptr(m2)[9] = 26.f;
    glm::value_ptr(m2)[10] = 27.f;
    glm::value_ptr(m2)[11] = 28.f;
    glm::value_ptr(m2)[12] = 29.f;
    glm::value_ptr(m2)[13] = 30.f;
    glm::value_ptr(m2)[14] = 31.f;
    glm::value_ptr(m2)[15] = 32.f;
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
                        .offset = glm::vec3(5.f, 6.f, 7.f),
                        .transformation = m

                    }
                },
                .offset = glm::vec3(8.f, 9.f, 10.f),
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





TEST_CASE("Validate: Tracker/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "name": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Name/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "name": ""
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": 123
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Name/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "name": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Name/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "name": ""
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": {
          "vrpnaddress": 123,
          "id": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Address/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": {
          "vrpnaddress": "",
          "id": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Sensors/Identifier/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "sensors": {
          "vrpnaddress": "abc",
          "id": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": 123,
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Address/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": "",
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Count/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": "abc",
          "count": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Buttons/Count/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "buttons": {
          "vrpnaddress": "abc",
          "count": -1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": 123
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Address/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": 123,
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Address/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": "",
          "count": 1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Count/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": "abc",
          "count": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Axes/Count/Illegal Value", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "axes": {
          "vrpnaddress": "abc",
          "count": -1
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": "abc"
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {}
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "y": 2,
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": "abc",
          "y": 2,
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "y": "abc",
          "z": 3
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "y": 2
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "offset": {
          "x": 1,
          "y": 2,
          "z": "abc"
        }
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Transformation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "matrix": "abc"
      }
    ]
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Device/Transformation/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "matrix": []
      }
    ]
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "devices": [
      {
        "matrix": [ 1.0, 2.0, 3.0, 4.0 ]
      }
    ]
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}

TEST_CASE("Validate: Tracker/Offset/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/All/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {}
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/X/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/X/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": "abc",
      "y": 2,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Y/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Y/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "y": "abc",
      "z": 3
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Z/Missing", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "y": 2
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Offset/Z/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "offset": {
      "x": 1,
      "y": 2,
      "z": "abc"
    }
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Scale/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "scale": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Transformation/Wrong Type", "[validate]") {
    constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "matrix": "abc"
  }
}
)";

    CHECK_THROWS_AS(validate(Config), ParsingError);
}

TEST_CASE("Validate: Tracker/Transformation/Illegal Value", "[validate]") {
    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "matrix": []
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }

    {
        constexpr std::string_view Config = R"(
{
  "version": 1,
  "masteraddress": "localhost",
  "trackers": {
    "matrix": [ 1.0, 2.0, 3.0, 4.0 ]
  }
}
)";

        CHECK_THROWS_AS(validate(Config), ParsingError);
    }
}
