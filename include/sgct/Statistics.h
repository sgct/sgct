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

#define STATS_HISTORY_length 512

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
	double FrameTime[STATS_HISTORY_length];
	double DrawTime[STATS_HISTORY_length];
	double SyncTime[STATS_HISTORY_length];
};

} //core_sgct

#endif