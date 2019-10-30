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




#if 0

#include <sgct/shaderprogram.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace sgct::core {

/**
 * Helper class for measuring application statistics
 */
class Statistics {
public:
    Statistics();
    ~Statistics();
    void initVBO();
    void setAvgFPS(float afps);
    void setFrameTime(float t);
    void setDrawTime(float t);
    void setSyncTime(float t);

    /// Set the minimum and maximum time it takes for a sync message from send to receive
    void setLoopTime(float min, float max);
    void addSyncTime(float t);
    void update();
    void draw(float lineWidth);

    float getAvgFPS() const;
    float getAvgDrawTime() const;
    float getAvgSyncTime() const;
    float getAvgFrameTime() const;
    float getMinFrameTime() const;
    float getMaxFrameTime() const;
    float getFrameTimeStandardDeviation() const;
    float getFrameTime() const;
    float getDrawTime() const;
    float getSyncTime() const;

private:
    static inline const int StatsHistoryLength = 512;
    static inline const int StatsAverageLength = 32;
    static inline const float VertScale = 5000.f;
    static inline const int StatsNumberOfDynamicObjs = 5;
    static inline const int StatsNumberOfStaticObjs = 3;

    float _avgFPS = 0.f;
    float _avgDrawTime = 0.f;
    float _avgSyncTime = 0.f;
    float _avgFrameTime = 0.f;
    float _minFrameTime = std::numeric_limits<float>::max();
    float _maxFrameTime = -std::numeric_limits<float>::max();
    float _stdDevFrameTime = 0.f;

    struct StatsVertex {
        float x;
        float y;
    };
    struct {
        std::array<StatsVertex, StatsHistoryLength> frameTime;
        std::array<StatsVertex, StatsHistoryLength> drawTime;
        std::array<StatsVertex, StatsHistoryLength> syncTime;
        std::array<StatsVertex, StatsHistoryLength> loopTimeMax;
        std::array<StatsVertex, StatsHistoryLength> loopTimeMin;
    } _dynamicVertexList;
    struct {
        glm::vec4 frameTime = glm::vec4(1.f, 1.f, 0.f, 0.8f);
        glm::vec4 drawTime = glm::vec4(1.f, 0.f, 1.f, 0.8f);
        glm::vec4 syncTime = glm::vec4(0.f, 1.f, 1.f, 0.8f);
        glm::vec4 loopTimeMax = glm::vec4(0.4f, 0.4f, 1.f, 0.8f);
        glm::vec4 loopTimeMin = glm::vec4(0.f, 0.f, 0.8f, 0.8f);
    } _dynamicColors;

    glm::vec4 _staticColorGrid = glm::vec4(1.f, 1.f, 1.f, 0.2f);
    glm::vec4 _staticColorFrequency = glm::vec4(1.f, 0.f, 0.f, 1.f);
    glm::vec4 _staticColorBackground = glm::vec4(0.f, 0.f, 0.f, 0.5f);

    // VBOs
    unsigned int _vboIndex = 0;
    // double buffered for ping-pong
    std::array<unsigned int, 2> _dynamicVAO = { 0, 0 };
    std::array<unsigned int, 2> _dynamicVBO = { 0, 0 };
    unsigned int _staticVAO = 0;
    unsigned int _staticVBO = 0;

    int _nLines = 0;

    ShaderProgram _shader;
    int _mvpLoc = -1;
    int _colorLoc = -1;

    std::vector<float> _staticVerts;
};

} // namespace sgct::core

#endif 

#endif // __SGCT__STATISTICS__H__
