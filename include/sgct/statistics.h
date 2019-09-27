/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__STATISTICS__H__
#define __SGCT__STATISTICS__H__

#include <sgct/shaderprogram.h>
#include <glm/glm.hpp>
#include <vector>

namespace sgct::core {

/**
 * Helper class for measuring application statistics
 */
class Statistics {
public:
    Statistics();
    ~Statistics();
    void initVBO(bool fixedPipeline);
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

    float mAvgFPS = 0.f;
    float mAvgDrawTime = 0.f;
    float mAvgSyncTime = 0.f;
    float mAvgFrameTime = 0.f;
    float mMinFrameTime = std::numeric_limits<float>::max();
    float mMaxFrameTime = -std::numeric_limits<float>::max();
    float mStdDevFrameTime = 0.f;

    struct StatsVertex {
        float x;
        float y;
    };
    struct {
        StatsVertex frameTime[StatsHistoryLength];
        StatsVertex drawTime[StatsHistoryLength];
        StatsVertex syncTime[StatsHistoryLength];
        StatsVertex loopTimeMax[StatsHistoryLength];
        StatsVertex loopTimeMin[StatsHistoryLength];
    } mDynamicVertexList;
    struct {
        glm::vec4 frameTime = glm::vec4(1.f, 1.f, 0.f, 0.8f);
        glm::vec4 drawTime = glm::vec4(1.f, 0.f, 1.f, 0.8f);
        glm::vec4 syncTime = glm::vec4(0.f, 1.f, 1.f, 0.8f);
        glm::vec4 loopTimeMax = glm::vec4(0.4f, 0.4f, 1.f, 0.8f);
        glm::vec4 loopTimeMin = glm::vec4(0.f, 0.f, 0.8f, 0.8f);
    } mDynamicColors;

    glm::vec4 mStaticColorGrid = glm::vec4(1.f, 1.f, 1.f, 0.2f);
    glm::vec4 mStaticColorFrequency = glm::vec4(1.f, 0.f, 0.f, 1.f);
    glm::vec4 mStaticColorBackground = glm::vec4(0.f, 0.f, 0.f, 0.5f);

    //VBOs
    unsigned int mVBOIndex = 0;
    // double buffered for ping-pong
    unsigned int mDynamicVBO[2] = { 0, 0 };
    unsigned int mDynamicVAO[2] = { 0, 0 };
    unsigned int mStaticVBO = 0;
    unsigned int mStaticVAO = 0;

    int mNumberOfLines = 0;
    bool mFixedPipeline = true;

    ShaderProgram mShader;
    int mMVPLoc = -1;
    int mColLoc = -1;

    std::vector<float> mStaticVerts;
};

} //sgct_core

#endif // __SGCT__STATISTICS__H__
