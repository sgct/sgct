/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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

struct StatsVertex
{
	double x, y;
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
	void setAvgFPS(double afps);
	void setFrameTime(double t);
	void setDrawTime(double t);
	void setSyncTime(double t);
	void setLoopTime(double min, double max);
	void addSyncTime(double t);
	void update();
	void draw(float lineWidth);

	const double & getAvgFPS() { return mAvgFPS; }
	const double & getAvgDrawTime() { return mAvgDrawTime; }
	const double & getAvgSyncTime() { return mAvgSyncTime; }
	const double & getAvgFrameTime() { return mAvgFrameTime; }
	const double & getFrameTime() { return mDynamicVertexList[FRAME_TIME * STATS_HISTORY_LENGTH].y; }
	const double & getDrawTime() { return mDynamicVertexList[DRAW_TIME * STATS_HISTORY_LENGTH].y; }
	const double & getSyncTime() { return mDynamicVertexList[SYNC_TIME * STATS_HISTORY_LENGTH].y; }

private:
	double mAvgFPS;
	double mAvgDrawTime;
	double mAvgSyncTime;
	double mAvgFrameTime;
	StatsVertex mDynamicVertexList[STATS_HISTORY_LENGTH * STATS_NUMBER_OF_DYNAMIC_OBJS];
	glm::vec4 mDynamicColors[STATS_NUMBER_OF_DYNAMIC_OBJS];
	glm::vec4 mStaticColors[STATS_NUMBER_OF_STATIC_OBJS];

	//VBOs
    unsigned int mVBOIndex;
	unsigned int mDynamicVBO[2]; //double buffered for ping-pong
	unsigned int mDynamicVAO[2]; //double buffered for ping-pong
	unsigned int mStaticVBOs[STATS_NUMBER_OF_STATIC_OBJS];
	unsigned int mStaticVAOs[STATS_NUMBER_OF_STATIC_OBJS];

	size_t mNumberOfLineVerts;
	bool mFixedPipeline;

	sgct::ShaderProgram mShader;
	int mMVPLoc, mColLoc;
};

} //sgct_core

#endif
