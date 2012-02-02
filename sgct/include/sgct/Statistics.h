#ifndef _STATISTICS_H_
#define _STATISTICS_H_

namespace core_sgct
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

} //core_sgct

#endif