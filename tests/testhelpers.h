/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__TESTHELPERS__H__
#define __SGCT__TESTHELPERS__H__

#include <catch2/catch_test_macros.hpp>

#include <sgct/error.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>

namespace sgct::test {

/**
 * Creates a file with the provided content in the temporary directory. The file is
 * removed when the object goes out of scope. The name gets a random prefix so that
 * tests running in parallel do not interfere with each other.
 */
class TempFile {
public:
    TempFile(std::string_view name, std::string_view content) {
        const unsigned int id = std::random_device()();
        _path = std::filesystem::temp_directory_path() /
            ("sgct-test-" + std::to_string(id) + "-" + std::string(name));
        std::ofstream file = std::ofstream(_path, std::ofstream::binary);
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(_path, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    const std::filesystem::path& path() const {
        return _path;
    }

private:
    std::filesystem::path _path;
};

/**
 * Calls the function \p f and checks that it throws an sgct::Error with the provided
 * \p component and \p code.
 */
template <typename F>
void checkError(F&& f, Error::Component component, int code) {
    try {
        f();
        FAIL("No exception was thrown");
    }
    catch (const Error& e) {
        CHECK(e.component == component);
        CHECK(e.code == code);
    }
}

} // namespace sgct::test

#endif // __SGCT__TESTHELPERS__H__
