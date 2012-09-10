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

#ifndef _SGCT_READ_CONFIG
#define _SGCT_READ_CONFIG

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace core_sgct //simple graphics cluster toolkit
{

class ReadConfig
{
public:
	ReadConfig( const std::string filename );

	enum StereoMode { NoStereo = 0, Active, Anaglyph_Red_Cyan, Anaglyph_Amber_Blue, Checkerboard, Checkerboard_Inverted };

	bool isValid() { return valid; }
	bool isExternalControlPortSet() { return useExternalControlPort; }
	bool isMasterSyncLocked() { return useMasterSyncLock; }
	const glm::vec3 * getSceneOffset() { return &sceneOffset; }
	const float & getYaw() { return mYaw; }
	const float & getPitch() { return mPitch; }
	const float & getRoll() { return mRoll; }

	//font stuff
	const int & getFontSize() { return mFontSize; }
	const std::string getFontName() { return mFontName; }
	const std::string getFontPath() { return mFontPath; }

private:
    bool replaceEnvVars( const std::string &filename );
	void readAndParseXML();
	int getStereoType( const std::string type );

	bool valid;
	bool useMasterSyncLock;
	bool useExternalControlPort;
	std::string xmlFileName;
	//fontdata
	std::string mFontName;
	std::string mFontPath;
	int mFontSize;
	glm::vec3 sceneOffset;
	float mYaw, mPitch, mRoll;
};

}

#endif
