/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#define STATS_HISTORY_LENGTH 512
#define STATS_AVERAGE_LENGTH 32
#define VERT_SCALE 10000.0f
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
	void draw(unsigned int frameNumber, float lineWidth);

	const double & getAvgFPS() { return mAvgFPS; }
	const double & getAvgDrawTime() { return mAvgDrawTime; }
	const double & getAvgSyncTime() { return mAvgSyncTime; }
	const double & getAvgFrameTime() { return mAvgFrameTime; }
	const double & getFrameTime() { return mFrameTime[0].y; }
	const double & getDrawTime() { return mDrawTime[0].y; }
	const double & getSyncTime() { return mSyncTime[0].y; }

private:
	StatsVertex * getVerts( unsigned int index );

	double mAvgFPS;
	double mAvgDrawTime;
	double mAvgSyncTime;
	double mAvgFrameTime;
	StatsVertex mLoopTimeMax[STATS_HISTORY_LENGTH];
	StatsVertex mLoopTimeMin[STATS_HISTORY_LENGTH];
	StatsVertex mFrameTime[STATS_HISTORY_LENGTH];
	StatsVertex mDrawTime[STATS_HISTORY_LENGTH];
	StatsVertex mSyncTime[STATS_HISTORY_LENGTH];
	enum mStatsDynamicType { FRAME_TIME = 0, DRAW_TIME, SYNC_TIME, LOOP_TIME_MAX, LOOP_TIME_MIN };
	enum mStatsStaticType { GRID = 0, FREQ, BG };
	unsigned int mDynamicVBOs[STATS_NUMBER_OF_DYNAMIC_OBJS];
	unsigned int mDynamicVAOs[STATS_NUMBER_OF_DYNAMIC_OBJS];
	glm::vec4 mDynamicColors[STATS_NUMBER_OF_DYNAMIC_OBJS];
	
	unsigned int mStaticVBOs[STATS_NUMBER_OF_STATIC_OBJS];
	unsigned int mStaticVAOs[STATS_NUMBER_OF_STATIC_OBJS];
	glm::vec4 mStaticColors[STATS_NUMBER_OF_STATIC_OBJS];

	size_t mNumberOfLineVerts;
	bool mFixedPipeline;

	sgct::ShaderProgram mShader;
	int mMVPLoc, mColLoc;
};

} //sgct_core

#endif
