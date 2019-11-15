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

namespace sgct::core {

class StatisticsRenderer {
public:
    StatisticsRenderer(const Engine::Statistics& statistics);
    ~StatisticsRenderer();

    void update();
    void render();

private:
    const Engine::Statistics& _statistics;

    ShaderProgram _shader;
    int _mvpLoc = -1;
    int _colorLoc = -1;
    struct {
        unsigned int vao = 0;
        unsigned int vbo = 0;
        int nLines = 0;
    } _static;

    struct {
        unsigned int vao = 0;
        unsigned int vbo = 0;
    } _dynamic;

    struct Vertex {
        float x;
        float y;
    };
    struct Vertices {
        std::array<Vertex, Engine::Statistics::HistoryLength> frametimes;
        std::array<Vertex, Engine::Statistics::HistoryLength> drawTimes;
        std::array<Vertex, Engine::Statistics::HistoryLength> syncTimes;
        std::array<Vertex, Engine::Statistics::HistoryLength> loopTimeMin;
        std::array<Vertex, Engine::Statistics::HistoryLength> loopTimeMax;
    };
    std::unique_ptr<Vertices> _vertexBuffer;
};

} // namespace sgct::core

#endif // __SGCT__STATISTICS__H__
