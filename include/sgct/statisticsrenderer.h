/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__STATISTICS__H__
#define __SGCT__STATISTICS__H__

#include <sgct/engine.h>
#include <sgct/shaderprogram.h>
#include <memory>

namespace sgct { class Window; }

namespace sgct::core {

class StatisticsRenderer {
public:
    StatisticsRenderer(const Engine::Statistics& statistics);
    ~StatisticsRenderer();

    void update();
    void render(Window& window);

private:
    const Engine::Statistics& _statistics;

    ShaderProgram _shader;
    int _mvpLoc = -1;
    int _colorLoc = -1;

    struct Vertex {
        float x = 0.f;
        float y = 0.f;
    };
    struct Lines {
        struct {
            unsigned int vao = 0;
            unsigned int vbo = 0;
            int nLines = 0;
        } staticDraw;

        struct {
            unsigned int vao = 0;
            unsigned int vbo = 0;
        } dynamicDraw;


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
        static const int Bins = 128;

        struct Values {
            std::array<int, Bins> frametimes;
            std::array<int, Bins> drawTimes;
            std::array<int, Bins> syncTimes;
            std::array<int, Bins> loopTimeMin;
            std::array<int, Bins> loopTimeMax;
        };
        Values values;

        struct Vertices {
            // The implementation requires frametimes to be the first element so beware
            std::array<Vertex, 6 * Bins> frametimes;
            std::array<Vertex, 6 * Bins> drawTimes;
            std::array<Vertex, 6 * Bins> syncTimes;
            std::array<Vertex, 6 * Bins> loopTimeMin;
            std::array<Vertex, 6 * Bins> loopTimeMax;
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
            unsigned int vao = 0;
            unsigned int vbo = 0;
        } staticDraw;

        struct {
            unsigned int vao = 0;
            unsigned int vbo = 0;
        } dynamicDraw;
    };
    Histogram _histogram;
};

} // namespace sgct::core

#endif // __SGCT__STATISTICS__H__
