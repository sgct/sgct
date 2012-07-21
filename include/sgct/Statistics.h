/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.
 
Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#define STATS_HISTORY_LENGTH 512
#define VERT_SCALE 10000.0f

struct StatsVertex
{
	double x, y;
};

namespace core_sgct
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

} //core_sgct

#endif