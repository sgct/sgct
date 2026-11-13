/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/


#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "schema.h"
#include <sgct/config.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
    std::vector<std::filesystem::path> exampleFiles() {
        std::vector<std::filesystem::path> res;
        const std::filesystem::path folder = std::string(BASE_PATH) + "/config";
        for (const std::filesystem::directory_entry& e :
             std::filesystem::directory_iterator(folder))
        {
            if (e.is_regular_file() && e.path().extension() == ".json") {
                res.push_back(e.path());
            }
        }
        return res;
    }

    // The configuration files of the image tests, which are run by support/runner
    std::vector<std::filesystem::path> visualTestFiles() {
        std::vector<std::filesystem::path> res;
        const std::filesystem::path folder = std::string(BASE_PATH) + "/visualtests";
        for (const std::filesystem::directory_entry& e :
             std::filesystem::recursive_directory_iterator(folder))
        {
            if (e.is_regular_file() && e.path().extension() == ".json") {
                res.push_back(e.path());
            }
        }
        return res;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream f = std::ifstream(path);
        return std::string(
            std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>()
        );
    }
} // namespace

TEST_CASE("Example files exist", "[roundtrip]") {
    CHECK_FALSE(exampleFiles().empty());
}

TEST_CASE("Visual test files exist", "[roundtrip]") {
    CHECK_FALSE(visualTestFiles().empty());
}

TEST_CASE("Roundtrip: Example and visual test files", "[roundtrip]") {
    const std::filesystem::path path = GENERATE(
        from_range(exampleFiles()),
        from_range(visualTestFiles())
    );
    CAPTURE(path.generic_string());

    CHECK_NOTHROW(validate(readFile(path)));

    const sgct::config::Cluster cluster = sgct::readConfig(path);
    CHECK_NOTHROW(sgct::config::validateCluster(cluster));

    const std::string serialized = sgct::serializeConfig(cluster);
    CAPTURE(serialized);
    CHECK_NOTHROW(validate(serialized));

    const sgct::config::Cluster roundtrip = sgct::readJsonConfig(serialized);
    CHECK(roundtrip == cluster);

    // Serializing a second time has to produce the identical string
    CHECK(sgct::serializeConfig(roundtrip) == serialized);
}
