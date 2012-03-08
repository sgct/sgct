#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#define STATS_HISTORY_LENGHT 512

namespace core_sgct
{

/*!
Helper struct for measuring application statistics
*/
class Statistics
{
public:
	Statistics();
	void setAvgFPS(double afps);
	void setFrameTime(double t);
	void setDrawTime(double t);
	void setSyncTime(double t);
	void addSyncTime(double t);
	void draw();

	const double & getAvgFPS() { return AvgFPS; }
	const double & getFrameTime() { return FrameTime[0]; }
	const double & getDrawTime() { return DrawTime[0]; }
	const double & getSyncTime() { return SyncTime[0]; }

private:
	double AvgFPS;
	double FrameTime[STATS_HISTORY_LENGHT];
	double DrawTime[STATS_HISTORY_LENGHT];
	double SyncTime[STATS_HISTORY_LENGHT];
};

} //core_sgct

#endif