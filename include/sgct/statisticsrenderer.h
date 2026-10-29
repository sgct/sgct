/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__STATISTICSRENDERER__H__
#define __SGCT__STATISTICSRENDERER__H__

#include <sgct/sgctexports.h>

#include <sgct/engine.h>
#include <sgct/math.h>
#include <sgct/vulkan/buffer.h>
#include <sgct/vulkan/pipeline.h>
#include <vulkan/vulkan.h>
#include <array>
#include <map>
#include <memory>

namespace sgct {

class Viewport;
class Window;

/**
 * Renders the frame time graphs and histograms of the Engine::Statistics into the 2D
 * pass of each viewport. The graph data is written by #update once per frame into one of
 * several host-visible vertex buffers, so that frames that are still in flight keep
 * reading the data they were recorded with.
 */
class SGCT_EXPORT StatisticsRenderer {
public:
    StatisticsRenderer(const Engine::Statistics& statistics);
    ~StatisticsRenderer();

    void update();

    /**
     * Records the statistics into the command buffer of the 2D pass of the \p window,
     * whose viewport and scissor are set to the area of \p viewport.
     */
    void render(const Window& window, const Viewport& viewport);

    float scale() const;
    void setScale(float scale);

    vec2 offset() const;
    void setOffset(vec2 offset);

private:
    // The pipelines for the different primitive topologies, created for a render pass
    struct Pipelines {
        std::unique_ptr<vulkan::Pipeline> triangleStrip;
        std::unique_ptr<vulkan::Pipeline> triangleList;
        std::unique_ptr<vulkan::Pipeline> lineList;
        std::unique_ptr<vulkan::Pipeline> lineStrip;
    };
    const Pipelines& pipelines(VkRenderPass renderPass,
        const vulkan::SceneFormat& format);

    void draw(VkCommandBuffer cmd, const vulkan::Pipeline& pipeline, VkBuffer buffer,
        const vec4& color, const mat4& mvp, uint32_t first, uint32_t count) const;

    const Engine::Statistics& _statistics;

    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    /// The pipelines for the color formats of the render targets that are drawn into
    std::map<vulkan::SceneFormat, Pipelines> _pipelines;

    /// The background, grid lines, and histogram axes; see the constructor for the layout
    std::unique_ptr<vulkan::Buffer> _staticBuffer;
    /// The lines of the graphs followed by the histogram bars
    static constexpr int DynamicBufferCount = 3;
    std::array<std::unique_ptr<vulkan::Buffer>, DynamicBufferCount> _dynamicBuffers;
    /// The dynamic buffer that was last written by #update
    int _currentDynamicBuffer = 0;

    struct Vertex {
        float x = 0.f;
        float y = 0.f;
    };
    struct Lines {
        struct {
            int nLines = 0;
        } staticDraw;


        struct Vertices {
            // The implementation requires frametimes to be the first element so beware
            std::array<Vertex, Engine::Statistics::HistoryLength> frametimes;
            std::array<Vertex, Engine::Statistics::HistoryLength> drawTimes;
            std::array<Vertex, Engine::Statistics::HistoryLength> syncTimes;
            std::array<Vertex, Engine::Statistics::HistoryLength> loopTimeMin;
            std::array<Vertex, Engine::Statistics::HistoryLength> loopTimeMax;
        };
        Vertices buffer;
    };
    Lines _lines;

    struct Histogram {
        // Each bin covers 1ms
        static constexpr int Bins = 128;

        struct Values {
            std::array<int, Bins> frametimes = {};
            std::array<int, Bins> drawTimes = {};
            std::array<int, Bins> syncTimes = {};
            std::array<int, Bins> loopTimeMin = {};
            std::array<int, Bins> loopTimeMax = {};
        };
        Values values;

        struct Vertices {
            // The implementation requires frametimes to be the first element so beware
            std::array<Vertex, 6 * Bins> frametimes = {};
            std::array<Vertex, 6 * Bins> drawTimes = {};
            std::array<Vertex, 6 * Bins> syncTimes = {};
            std::array<Vertex, 6 * Bins> loopTimeMin = {};
            std::array<Vertex, 6 * Bins> loopTimeMax = {};
        };
        Vertices buffer;

        struct {
            int frametimes = 0;
            int drawTimes = 0;
            int syncTimes = 0;
            int loopTimeMin = 0;
            int loopTimeMax = 0;
        } maxBinValue;

        struct {
            /// The index of the first vertex of the histogram axes in the static buffer
            uint32_t first = 0;
        } staticDraw;
    };
    Histogram _histogram;

    float _scale = 0.5f;
    vec2 _offset;
};

} // namespace sgct

#endif // __SGCT__STATISTICSRENDERER__H__
