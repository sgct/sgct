/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#define STATS_HISTORY_LENGTH 512
#define STATS_AVERAGE_LENGTH 32
#define VERT_SCALE 5000.0f
#define STATS_NUMBER_OF_DYNAMIC_OBJS 5
#define STATS_NUMBER_OF_STATIC_OBJS 3

#include "ShaderProgram.h"
#include <glm/glm.hpp>
#include <vector>

struct StatsVertex
{
	float x, y;
};

namespace sgct_core
{

/*!
Helper class for measuring application statistics
*/
class Statistics
{
private:
	enum mStatsDynamicType { FRAME_TIME = 0, DRAW_TIME = 1, SYNC_TIME = 2, LOOP_TIME_MAX = 3, LOOP_TIME_MIN = 4 };
	enum mStatsStaticType { GRID = 0, FREQ, BG };

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

	const float getAvgFPS() { return mAvgFPS; }
	const float getAvgDrawTime() { return mAvgDrawTime; }
	const float getAvgSyncTime() { return mAvgSyncTime; }
	const float getAvgFrameTime() { return mAvgFrameTime; }
	const float getFrameTime() { return mDynamicVertexList[FRAME_TIME * STATS_HISTORY_LENGTH].y; }
	const float getDrawTime() { return mDynamicVertexList[DRAW_TIME * STATS_HISTORY_LENGTH].y; }
	const float getSyncTime() { return mDynamicVertexList[SYNC_TIME * STATS_HISTORY_LENGTH].y; }

private:
	float mAvgFPS;
	float mAvgDrawTime;
	float mAvgSyncTime;
	float mAvgFrameTime;
	StatsVertex mDynamicVertexList[STATS_HISTORY_LENGTH * STATS_NUMBER_OF_DYNAMIC_OBJS];
	glm::vec4 mDynamicColors[STATS_NUMBER_OF_DYNAMIC_OBJS];
	glm::vec4 mStaticColors[STATS_NUMBER_OF_STATIC_OBJS];

	//VBOs
    unsigned int mVBOIndex;
	unsigned int mDynamicVBO[2]; //double buffered for ping-pong
	unsigned int mDynamicVAO[2]; //double buffered for ping-pong
	unsigned int mStaticVBO;
	unsigned int mStaticVAO;

	int mNumberOfLines;
	bool mFixedPipeline;

	sgct::ShaderProgram mShader;
	int mMVPLoc, mColLoc;

	std::vector<float> mStaticVerts;
};

} //sgct_core

#endif
