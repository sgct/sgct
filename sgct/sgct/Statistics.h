#ifndef _STATISTICS_H_
#define _STATISTICS_H_

namespace sgct
{

/*!
Helper struct for measuring application statistics
*/
struct Statistics
{
	double FPS;
	double AvgFPS;
	double FrameTime;
	double DrawTime;
	double TotalTime;
};

} //sgct

#endif