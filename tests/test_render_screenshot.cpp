/*****************************************************************************************\
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <sgct/image.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>

using namespace sgct;

namespace {

/// The number of screenshots the calibrator's -runTests sequence captures
constexpr int NumScreenshots = 4;
/// The maximum per-channel difference a pixel may have and still count as matching
constexpr int Tolerance = 2;
/// The maximum fraction of pixels that may differ before the comparison fails
constexpr double MaxDifferingFraction = 0.001;

/// Counts the pixels of `a` that differ from `b` by more than `tolerance` in any channel.
/// The images must have the same size and channel count (checked by the caller).
size_t countDifferingPixels(const Image& a, const Image& b, int tolerance) {
    const int channels = a.channels();
    const size_t totalPixels = static_cast<size_t>(a.size().x) * a.size().y;
    size_t differing = 0;
    for (size_t p = 0; p < totalPixels; ++p) {
        for (int c = 0; c < channels; ++c) {
            const int va = a.data()[p * channels + c];
            const int vb = b.data()[p * channels + c];
            if (std::abs(va - vb) > tolerance) {
                ++differing;
                break;
            }
        }
    }
    return differing;
}

} // namespace

TEST_CASE("Render: Screenshots match references", "[render][screenshot]") {
    const std::filesystem::path referenceDir =
        std::filesystem::path(BASE_PATH) / "tests" / "reference";
    const std::filesystem::path renderDir = RENDER_TEST_DIR;

    // The generated screenshots are produced by the CalibratorRenderSmoke ctest, which runs
    // the calibrator with its working directory set to RENDER_TEST_DIR. If they are missing
    // (e.g. a headless machine without a Vulkan device), skip the comparison.
    if (!std::filesystem::exists(renderDir / "win0_000000.png")) {
        SKIP("No generated screenshots found (calibrator not run?); skipping comparison");
    }

    for (int i = 0; i < NumScreenshots; ++i) {
        const std::string name = std::format("win0_{:06}.png", i);

        Image reference;
        Image generated;
        REQUIRE_NOTHROW(reference.load(referenceDir / name));
        REQUIRE_NOTHROW(generated.load(renderDir / name));

        const ivec2 refSize = reference.size();
        const ivec2 genSize = generated.size();
        INFO(std::format(
            "{}: reference {}x{} ({}ch), generated {}x{} ({}ch)", name, refSize.x, refSize.y,
            reference.channels(), genSize.x, genSize.y, generated.channels()
        ));
        REQUIRE(genSize == refSize);
        REQUIRE(generated.channels() == reference.channels());

        const size_t differing = countDifferingPixels(reference, generated, Tolerance);
        const size_t totalPixels = static_cast<size_t>(refSize.x) * refSize.y;
        const double fraction = static_cast<double>(differing) / totalPixels;
        INFO(std::format(
            "{}: {} of {} pixels differ beyond tolerance {} ({:.4f}%)", name, differing,
            totalPixels, Tolerance, fraction * 100.0
        ));
        REQUIRE(fraction < MaxDifferingFraction);
    }
}
