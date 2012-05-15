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

#define TIXML_USE_STL //needed for tinyXML lib to link properly in mingw

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/ReadConfig.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include <tinyxml.h>

core_sgct::ReadConfig::ReadConfig( const std::string filename )
{
	valid = false;
	useExternalControlPort = false;
	useMasterSyncLock = false;
	sceneOffset = glm::vec3(0.0f, 0.0f, 0.0f);
	mYaw = 0.0f;
	mPitch = 0.0f;
	mRoll = 0.0f;

	if( filename.empty() )
	{
		sgct::MessageHandler::Instance()->print("Error: No XML config file loaded.\n");
		return;
	}
	xmlFileName = filename;

	try
	{
		readAndParseXML();
	}
	catch(const char * err)
	{
		sgct::MessageHandler::Instance()->print("Error occured while reading config file '%s'\nError: %s\n", xmlFileName.c_str(), err);
		return;
	}
	valid = true;
	sgct::MessageHandler::Instance()->print("Config file '%s' read successfully!\n", xmlFileName.c_str());
	sgct::MessageHandler::Instance()->print("Number of nodes in cluster: %d\n",
		ClusterManager::Instance()->getNumberOfNodes());
	for(unsigned int i = 0; i<ClusterManager::Instance()->getNumberOfNodes(); i++)
		sgct::MessageHandler::Instance()->print("Node(%d) ip: %s [%s]\n", i,
		ClusterManager::Instance()->getNodePtr(i)->ip.c_str(),
		ClusterManager::Instance()->getNodePtr(i)->port.c_str());
}

void core_sgct::ReadConfig::readAndParseXML()
{
	TiXmlDocument initVals( xmlFileName.c_str() );
	if( !initVals.LoadFile() )
	{
		throw "Invalid XML file!";
	}

	TiXmlElement* XMLroot = initVals.FirstChildElement( "Cluster" );
	if( XMLroot == NULL )
	{
		throw "Cannot find XML root!";
	}

	std::string tmpStr( XMLroot->Attribute( "masterAddress" ) );
	ClusterManager::Instance()->setMasterIp( tmpStr );

	if( XMLroot->Attribute( "externalControlPort" ) != NULL )
	{
		tmpStr.assign( XMLroot->Attribute( "externalControlPort" ) );
		ClusterManager::Instance()->setExternalControlPort(tmpStr);
		useExternalControlPort = true;
	}

	if( XMLroot->Attribute( "lockMasterSync" ) != NULL )
	{
		useMasterSyncLock = strcmp( XMLroot->Attribute( "lockMasterSync" ), "true" ) == 0 ? true : false;
	}

	TiXmlElement* element[10];
	const char * val[10];
	element[0] = XMLroot->FirstChildElement();
	while( element[0] != NULL )
	{
		val[0] = element[0]->Value();

        if( strcmp("Scene", val[0]) == 0 )
        {
            element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();

				if( strcmp("Offset", val[1]) == 0 )
				{
				    double tmpOffset[] = {0.0, 0.0, 0.0};
					if( element[1]->Attribute("x", &tmpOffset[0] ) != NULL &&
                        element[1]->Attribute("y", &tmpOffset[1] ) != NULL &&
                        element[1]->Attribute("z", &tmpOffset[2] ) != NULL)
                    {
                        sceneOffset.x = static_cast<float>(tmpOffset[0]);
                        sceneOffset.y = static_cast<float>(tmpOffset[1]);
                        sceneOffset.z = static_cast<float>(tmpOffset[2]);
                        sgct::MessageHandler::Instance()->print("Setting scene offset to (%f, %f, %f)\n",
                                                                sceneOffset.x,
                                                                sceneOffset.y,
                                                                sceneOffset.z);
                    }
				}
				else if( strcmp("Orientation", val[1]) == 0 )
				{
					double tmpOrientation[] = {0.0, 0.0, 0.0};
					if( element[1]->Attribute("yaw", &tmpOrientation[0] ) != NULL &&
                        element[1]->Attribute("pitch", &tmpOrientation[1] ) != NULL &&
                        element[1]->Attribute("roll", &tmpOrientation[2] ) != NULL)
                    {
                        mYaw = glm::radians( static_cast<float>(tmpOrientation[0]) );
                        mPitch = glm::radians( static_cast<float>(tmpOrientation[1]) );
                        mRoll = glm::radians( static_cast<float>(tmpOrientation[2]) );
                        sgct::MessageHandler::Instance()->print("Setting scene orientation to (%f, %f, %f) radians\n",
                                                                mYaw,
                                                                mPitch,
                                                                mRoll);
                    }
				}

				//iterate
				element[1] = element[1]->NextSiblingElement();
			}
        }
		else if( strcmp("Node", val[0]) == 0 )
		{
			SGCTNode tmpNode;
			tmpNode.ip.assign( element[0]->Attribute( "ip" ) );
			tmpNode.port.assign( element[0]->Attribute( "port" ) );

			element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();
				if( strcmp("Window", val[1]) == 0 )
				{
					if( element[1]->Attribute("fullscreen") != NULL )
						tmpNode.getWindowPtr()->setWindowMode( strcmp( element[1]->Attribute("fullscreen"), "true" ) == 0 ? GLFW_FULLSCREEN : GLFW_WINDOW);

					int tmpSamples = 0;
					if( element[1]->Attribute("numberOfSamples", &tmpSamples ) != NULL )
						tmpNode.numberOfSamples = tmpSamples;

					if( element[1]->Attribute("swapLock") != NULL )
						tmpNode.getWindowPtr()->useSwapGroups(strcmp( element[1]->Attribute("swapLock"), "true" ) == 0 ? true : false);

                    int tmpInterval = 0;
					if( element[1]->Attribute("swapInterval", &tmpInterval) != NULL )
						tmpNode.swapInterval = tmpInterval;

					element[2] = element[1]->FirstChildElement();
					while( element[2] != NULL )
					{
						val[2] = element[2]->Value();
						int tmpWinData[4];
						memset(tmpWinData,0,4);

						if( strcmp("Stereo", val[2]) == 0 )
						{
							tmpNode.stereo = getStereoType( element[2]->Attribute("type") );
						}
						else if( strcmp("Pos", val[2]) == 0 )
						{
							element[2]->Attribute("x", &tmpWinData[0] );
							element[2]->Attribute("y", &tmpWinData[1] );
						}
						else if( strcmp("Size", val[2]) == 0 )
						{
							element[2]->Attribute("x", &tmpWinData[2] );
							element[2]->Attribute("y", &tmpWinData[3] );
						}

						tmpNode.getWindowPtr()->initWindowResolution(tmpWinData[2],tmpWinData[3]);
						tmpNode.getWindowPtr()->setWindowPosition(tmpWinData[0],tmpWinData[1]);

						//iterate
						element[2] = element[2]->NextSiblingElement();
					}
				}
				else if(strcmp("Viewport", val[1]) == 0)
				{
					Viewport tmpVp;

					if( element[1]->Attribute("overlay") != NULL )
						tmpVp.setOverlayTexture( element[1]->Attribute("overlay") );
					
					if( element[1]->Attribute("tracked") != NULL )
						tmpVp.setTracked( strcmp( element[1]->Attribute("tracked"), "true" ) == 0 ? true : false );

					//get eye if set
					if( element[1]->Attribute("eye") != NULL )
					{
						if( strcmp("both", element[1]->Attribute("eye")) == 0 )
						{
							tmpVp.setEye(Frustum::Mono);
						}
						else if( strcmp("left", element[1]->Attribute("eye")) == 0 )
						{
							tmpVp.setEye(Frustum::StereoLeftEye);
						}
						else if( strcmp("right", element[1]->Attribute("eye")) == 0 )
						{
							tmpVp.setEye(Frustum::StereoRightEye);
						}
					}

					element[2] = element[1]->FirstChildElement();
					while( element[2] != NULL )
					{
						val[2] = element[2]->Value();
						double dTmp[2];
						dTmp[0] = 0.0f;
						dTmp[1] = 0.0f;

						if(strcmp("Pos", val[2]) == 0)
						{
							element[2]->Attribute("x", &dTmp[0]);
							element[2]->Attribute("y", &dTmp[1]);
							tmpVp.setPos(static_cast<float>(dTmp[0]),
								static_cast<float>(dTmp[1]));
						}
						else if(strcmp("Size", val[2]) == 0)
						{
							element[2]->Attribute("x", &dTmp[0]);
							element[2]->Attribute("y", &dTmp[1]);
							tmpVp.setSize(static_cast<float>(dTmp[0]),
								static_cast<float>(dTmp[1]));
						}
						else if(strcmp("Viewplane", val[2]) == 0)
						{
							element[3] = element[2]->FirstChildElement();
							while( element[3] != NULL )
							{
								val[3] = element[3]->Value();

								if( strcmp("Pos", val[3]) == 0 )
								{
									glm::vec3 tmpVec;
									double dTmp[3];
									static unsigned int i=0;
									element[3]->Attribute("x", &dTmp[0]);
									element[3]->Attribute("y", &dTmp[1]);
									element[3]->Attribute("z", &dTmp[2]);

									tmpVec.x = static_cast<float>(dTmp[0]);
									tmpVec.y = static_cast<float>(dTmp[1]);
									tmpVec.z = static_cast<float>(dTmp[2]);

									tmpVp.setViewPlaneCoords(i%3, tmpVec);
									i++;
								}

								//iterate
								element[3] = element[3]->NextSiblingElement();
							}//end while level 3
						}//end if viewplane

					//iterate
					element[2] = element[2]->NextSiblingElement();
					}//end while level 2

					tmpNode.addViewport(tmpVp);
				}//end viewport

				//iterate
				element[1] = element[1]->NextSiblingElement();
			}

			ClusterManager::Instance()->addNode(tmpNode);
		}//end if node
		else if( strcmp("User", val[0]) == 0 )
		{
			double dTmp;
			element[0]->Attribute("eyeSeparation", &dTmp);
			ClusterManager::Instance()->getUserPtr()->setEyeSeparation(
				static_cast<float>( dTmp ));

			element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();

				if( strcmp("Pos", val[1]) == 0 )
				{
					double dTmp[3];
					element[1]->Attribute("x", &dTmp[0]);
					element[1]->Attribute("y", &dTmp[1]);
					element[1]->Attribute("z", &dTmp[2]);

					ClusterManager::Instance()->getUserPtr()->setPos(dTmp);
				}

				//iterate
				element[1] = element[1]->NextSiblingElement();
			}
		}
		else if( strcmp("Tracking", val[0]) == 0 )
		{
			if( element[0]->Attribute("vrpnAddress") != NULL )
			{
				ClusterManager::Instance()->getTrackingPtr()->connect( element[0]->Attribute("vrpnAddress") );

				int tmpi = -1;
				if( element[0]->Attribute("headSensorIndex") != NULL )
				{
					element[0]->Attribute("headSensorIndex", &tmpi);
					ClusterManager::Instance()->getTrackingPtr()->setHeadSensorIndex( tmpi );
				}
				else
					sgct::MessageHandler::Instance()->print("Info: Head sensor index not specified. Head tracking is disabled!\n");

				element[1] = element[0]->FirstChildElement();
				while( element[1] != NULL )
				{
					val[1] = element[1]->Value();

					if( strcmp("Offset", val[1]) == 0 )
					{
						double tmpd[3];
						element[1]->Attribute("x", &tmpd[0]);
						element[1]->Attribute("y", &tmpd[1]);
						element[1]->Attribute("z", &tmpd[2]);

						if( element[1]->Attribute("x") != NULL &&
							element[1]->Attribute("y") != NULL &&
							element[1]->Attribute("z") != NULL )
							ClusterManager::Instance()->getTrackingPtr()->setOffset( tmpd[0], tmpd[1], tmpd[2] );
					}
					else if( strcmp("Orientation", val[1]) == 0 )
					{
						double tmpd[3];
						element[1]->Attribute("x", &tmpd[0]);
						element[1]->Attribute("y", &tmpd[1]);
						element[1]->Attribute("z", &tmpd[2]);

						if( element[1]->Attribute("x") != NULL &&
							element[1]->Attribute("y") != NULL &&
							element[1]->Attribute("z") != NULL )
							ClusterManager::Instance()->getTrackingPtr()->setOrientation( tmpd[0], tmpd[1], tmpd[2] );
					}

					//iterate
					element[1] = element[1]->NextSiblingElement();
				}
			}
			else
				sgct::MessageHandler::Instance()->print("Error: No VRPN address provided! Tracking is disabled.\n");
		}// end tracking part

		//iterate
		element[0] = element[0]->NextSiblingElement();
	}
}

int core_sgct::ReadConfig::getStereoType( const std::string type )
{
	if( strcmp( type.c_str(), "none" ) == 0 )
		return None;
	else if( strcmp( type.c_str(), "active" ) == 0 )
		return Active;
	else if( strcmp( type.c_str(), "checkerboard" ) == 0 )
		return Checkerboard;
	else if( strcmp( type.c_str(), "checkerboard_inverted" ) == 0 )
		return Checkerboard_Inverted;
	else if( strcmp( type.c_str(), "anaglyph_red_cyan" ) == 0 )
		return Anaglyph_Red_Cyan;
	else if( strcmp( type.c_str(), "anaglyph_amber_blue" ) == 0 )
		return Anaglyph_Amber_Blue;

	//if match not found
	return -1;
}
