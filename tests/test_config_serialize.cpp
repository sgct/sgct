/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/


#include <catch2/catch_test_macros.hpp>

#include "schema.h"
#include <sgct/config.h>
#include <nlohmann/json.hpp>
#include <string>

using namespace sgct;
using namespace sgct::config;

namespace {
    Cluster clusterWithProjection(Projections projection) {
        Viewport viewport;
        viewport.projection = std::move(projection);
        Window window;
        window.size = glm::ivec2(640, 480);
        window.viewports.push_back(viewport);
        Node node;
        node.address = "localhost";
        node.port = 20401;
        node.windows.push_back(window);
        Cluster cluster;
        cluster.success = true;
        cluster.masterAddress = "localhost";
        cluster.users.emplace_back();
        cluster.nodes.push_back(node);
        return cluster;
    }

    nlohmann::json serializedViewport(const Cluster& cluster) {
        const nlohmann::json j = nlohmann::json::parse(serializeConfig(cluster));
        return j["nodes"][0]["windows"][0]["viewports"][0];
    }

    void checkColor(const nlohmann::json& j, const glm::vec4& color) {
        CHECK(j.size() == 4);
        CHECK(j.at("r").get<float>() == color.r);
        CHECK(j.at("g").get<float>() == color.g);
        CHECK(j.at("b").get<float>() == color.b);
        CHECK(j.at("a").get<float>() == color.a);
    }
} // namespace

TEST_CASE("Serialize: FisheyeProjection/Background", "[serialize]") {
    const glm::vec4 color = glm::vec4(0.25f, 0.5f, 0.75f, 1.f);
    const Cluster cluster = clusterWithProjection(
        FisheyeProjection{ .background = color }
    );
    checkColor(serializedViewport(cluster)["projection"]["background"], color);
    CHECK_NOTHROW(validate(serializeConfig(cluster)));
    CHECK(readJsonConfig(serializeConfig(cluster)) == cluster);
}

TEST_CASE("Serialize: SphericalMirrorProjection/Background", "[serialize]") {
    const glm::vec4 color = glm::vec4(0.25f, 0.5f, 0.75f, 1.f);
    const Cluster cluster = clusterWithProjection(
        SphericalMirrorProjection{
            .background = color,
            .mesh = { .bottom = "b.obj", .left = "l.obj", .right = "r.obj", .top = "t.obj" }
        }
    );
    checkColor(serializedViewport(cluster)["projection"]["background"], color);
    CHECK_NOTHROW(validate(serializeConfig(cluster)));
    CHECK(readJsonConfig(serializeConfig(cluster)) == cluster);
}

TEST_CASE("Serialize: Window/Scalable", "[serialize]") {
    Cluster cluster = clusterWithProjection(FisheyeProjection());
    cluster.nodes[0].windows[0].scalable = Window::Scalable{
        .mesh = std::filesystem::current_path() / "mesh.ol",
        .orthographicQuality = 1024,
        .orthographicResolution = 2048
    };

    const std::string str = serializeConfig(cluster);
    const nlohmann::json window = nlohmann::json::parse(str)["nodes"][0]["windows"][0];
    CHECK(window.at("scalablemesh_ortho_quality").get<int>() == 1024);
    CHECK(window.at("scalablemesh_ortho_res").get<int>() == 2048);
    CHECK_NOTHROW(validate(str));
    CHECK(readJsonConfig(str) == cluster);
}

TEST_CASE("Serialize: Viewport/Empty", "[serialize]") {
    const Cluster cluster = clusterWithProjection(NoProjection());

    const std::string str = serializeConfig(cluster);
    const nlohmann::json viewport = serializedViewport(cluster);
    CHECK(viewport.is_object());
    CHECK_FALSE(viewport.contains("projection"));
    CHECK_NOTHROW(validate(str));
    CHECK(readJsonConfig(str) == cluster);
}

TEST_CASE("Serialize: Window/Alpha", "[serialize]") {
    Cluster cluster = clusterWithProjection(NoProjection());
    cluster.nodes[0].windows[0].alpha = true;

    const std::string str = serializeConfig(cluster);
    const nlohmann::json window = nlohmann::json::parse(str)["nodes"][0]["windows"][0];
    CHECK(window.at("alpha").get<bool>() == true);
    CHECK_NOTHROW(validate(str));
    CHECK(readJsonConfig(str) == cluster);
}
