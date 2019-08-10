/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__STATISTICS__H__
#define __SGCT__STATISTICS__H__

#include <sgct/ShaderProgram.h>
#include <glm/glm.hpp>
#include <vector>

namespace sgct_core {

/*!
Helper class for measuring application statistics
*/
class Statistics {
private:
    enum mStatsDynamicType {
        FRAME_TIME = 0,
        DRAW_TIME = 1,
        SYNC_TIME = 2,
        LOOP_TIME_MAX = 3,
        LOOP_TIME_MIN = 4
    };
    enum mStatsStaticType {
        GRID = 0,
        FREQ,
        BG
    };

public:
    Statistics();
    ~Statistics();
    void initVBO(bool fixedPipeline);
    void setAvgFPS(float afps);
    void setFrameTime(float t);
    void setDrawTime(float t);
    void setSyncTime(float t);
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
    struct StatsVertex {
        float x;
        float y;
    };

    static inline const int StatsHistoryLength = 512;
    static inline const int StatsAverageLength = 32;
    static inline const float VertScale = 5000.f;
    static inline const int StatsNumberOfDynamicObjs = 5;
    static inline const int StatsNumberOfStaticObjs = 3;

    float mAvgFPS = 0.f;
    float mAvgDrawTime = 0.f;
    float mAvgSyncTime = 0.f;
    float mAvgFrameTime = 0.f;
    float mMinFrameTime;
    float mMaxFrameTime;
    float mStdDevFrameTime;
    StatsVertex mDynamicVertexList[StatsHistoryLength * StatsNumberOfDynamicObjs];
    glm::vec4 mDynamicColors[StatsNumberOfDynamicObjs];
    glm::vec4 mStaticColors[StatsNumberOfStaticObjs];

    //VBOs
    unsigned int mVBOIndex = 0;
    // double buffered for ping-pong
    unsigned int mDynamicVBO[2] = { 0, 0 };
    unsigned int mDynamicVAO[2] = { 0, 0 };
    unsigned int mStaticVBO = 0;
    unsigned int mStaticVAO = 0;

    int mNumberOfLines = 0;
    bool mFixedPipeline = true;

    sgct::ShaderProgram mShader;
    int mMVPLoc = -1;
    int mColLoc = -1;

    std::vector<float> mStaticVerts;
};

} //sgct_core

#endif // __SGCT__STATISTICS__H__
