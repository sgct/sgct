/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#define STATS_HISTORY_LENGTH 512
#define VERT_SCALE 10000.0f

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
	void initVBO();
	void setAvgFPS(double afps);
	void setFrameTime(double t);
	void setDrawTime(double t);
	void setSyncTime(double t);
	void addSyncTime(double t);
	void draw(unsigned long long frameNumber);

	const double & getAvgFPS() { return mAvgFPS; }
	const double & getFrameTime() { return mFrameTime[0].y; }
	const double & getDrawTime() { return mDrawTime[0].y; }
	const double & getSyncTime() { return mSyncTime[0].y; }

private:
	double mAvgFPS;
	StatsVertex mFrameTime[STATS_HISTORY_LENGTH];
	StatsVertex mDrawTime[STATS_HISTORY_LENGTH];
	StatsVertex mSyncTime[STATS_HISTORY_LENGTH];
	enum mStatsType { FRAME_TIME = 0, DRAW_TIME, SYNC_TIME };
	unsigned int mVboPtrs[4];
};

} //sgct_core

#endif
