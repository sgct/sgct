/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#define TIXML_USE_STL //needed for tinyXML lib to link properly in mingw
#define MAX_XML_DEPTH 16

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/ReadConfig.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#ifndef SGCT_DONT_USE_EXTERNAL
#include "../include/external/tinyxml2.h"
#else
#include <tinyxml2.h>
#endif

#include "../include/sgct/SGCTSettings.h"
#include <algorithm>
#include <sstream>

sgct_core::ReadConfig::ReadConfig( const std::string filename )
{
	valid = false;
    
	if( filename.empty() )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Error: No XML config file loaded.\n");
		return;
	}
	else
	{
	    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Parsing XML config '%s'...\n", filename.c_str());
	}
    
    if( !replaceEnvVars(filename) )
        return;
    
	if(!readAndParseXML())
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Error occured while reading config file '%s'\nError: %s\n", xmlFileName.c_str(), mErrorMsg.c_str());
		return;
	}
	valid = true;
    
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Config file '%s' read successfully!\n", xmlFileName.c_str());
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ReadConfig: Number of nodes in cluster: %d\n",
                                            ClusterManager::instance()->getNumberOfNodes());
    
	for(unsigned int i = 0; i<ClusterManager::instance()->getNumberOfNodes(); i++)
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "\tNode(%d) address: %s [%s]\n", i,
                                                ClusterManager::instance()->getNodePtr(i)->getAddress().c_str(),
                                                ClusterManager::instance()->getNodePtr(i)->getSyncPort().c_str());
}

bool sgct_core::ReadConfig::replaceEnvVars( const std::string &filename )
{
    size_t foundIndex = filename.find('%');
    if( foundIndex != std::string::npos )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Error: SGCT doesn't support the usage of '%%' characters in path or file name.\n");
		return false;
    }
    
    std::vector< size_t > beginEnvVar;
    std::vector< size_t > endEnvVar;
    
    foundIndex = 0;
    while( foundIndex != std::string::npos )
    {
        foundIndex = filename.find("$(", foundIndex);
        if(foundIndex != std::string::npos)
        {
            beginEnvVar.push_back(foundIndex);
            foundIndex = filename.find(')', foundIndex);
            if(foundIndex != std::string::npos)
                endEnvVar.push_back(foundIndex);
        }
    }
    
    if(beginEnvVar.size() != endEnvVar.size())
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Error: Bad configuration path string!\n");
		return false;
    }
    else
    {
        size_t appendPos = 0;
        for(unsigned int i=0; i<beginEnvVar.size(); i++)
        {
            xmlFileName.append(filename.substr(appendPos, beginEnvVar[i] - appendPos));
            std::string envVar = filename.substr(beginEnvVar[i] + 2, endEnvVar[i] - (beginEnvVar[i] + 2) );
            char * fetchedEnvVar = NULL;
            
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			size_t len;
			errno_t err = _dupenv_s( &fetchedEnvVar, &len, envVar.c_str() );
			if ( err )
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Error: Cannot fetch environment variable '%s'.\n", envVar.c_str());
				return false;
			}
#else
			fetchedEnvVar = getenv(envVar.c_str());
			if( fetchedEnvVar == NULL )
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Error: Cannot fetch environment variable '%s'.\n", envVar.c_str());
				return false;
			}
#endif
            
			xmlFileName.append( fetchedEnvVar );
            appendPos = endEnvVar[i]+1;
        }
        
        xmlFileName.append( filename.substr( appendPos ) );
        
        //replace all backslashes with slashes
        for(unsigned int i=0; i<xmlFileName.size(); i++)
            if(xmlFileName[i] == 92) //backslash
                xmlFileName[i] = '/';
    }
    
    return true;
}

bool sgct_core::ReadConfig::readAndParseXML()
{
	if (xmlFileName.empty())
	{
		mErrorMsg.assign("No XML file set!");
		return false;
	}
    
	tinyxml2::XMLDocument xmlDoc;
	if( xmlDoc.LoadFile(xmlFileName.c_str()) != tinyxml2::XML_NO_ERROR )
	{
        std::stringstream ss;
		if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2())
			ss << "Paring failed after: " << xmlDoc.GetErrorStr1() << " " << xmlDoc.GetErrorStr2();
		else
			ss << "File not found";
		mErrorMsg = ss.str();
		return false;
	}
    
	tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement( "Cluster" );
	if( XMLroot == NULL )
	{
		mErrorMsg.assign("Cannot find XML root!");
		return false;
	}
    
	const char * masterAddress = XMLroot->Attribute( "masterAddress" );
	if( masterAddress )
		ClusterManager::instance()->setMasterAddress( masterAddress );
	else
	{
		mErrorMsg.assign("Cannot find master address or DNS name in XML!");
		return false;
	}
    
	const char * debugMode = XMLroot->Attribute( "debug" );
	if( debugMode != NULL )
	{
		sgct::MessageHandler::instance()->setNotifyLevel( strcmp( debugMode, "true" ) == 0 ?
                                                         sgct::MessageHandler::NOTIFY_DEBUG : sgct::MessageHandler::NOTIFY_WARNING );
	}
    
	if( XMLroot->Attribute( "externalControlPort" ) != NULL )
	{
		std::string tmpStr( XMLroot->Attribute( "externalControlPort" ) );
		ClusterManager::instance()->setExternalControlPort(tmpStr);
	}
    
	if( XMLroot->Attribute( "firmSync" ) != NULL )
	{
		ClusterManager::instance()->setFirmFrameLockSyncStatus(
                                                               strcmp( XMLroot->Attribute( "firmSync" ), "true" ) == 0 ? true : false );
	}
    
	tinyxml2::XMLElement* element[MAX_XML_DEPTH];
	for(unsigned int i=0; i < MAX_XML_DEPTH; i++)
		element[i] = NULL;
	const char * val[MAX_XML_DEPTH];
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
				    float tmpOffset[] = {0.0f, 0.0f, 0.0f};
					if( element[1]->QueryFloatAttribute("x", &tmpOffset[0] ) == tinyxml2::XML_NO_ERROR &&
                       element[1]->QueryFloatAttribute("y", &tmpOffset[1] ) == tinyxml2::XML_NO_ERROR &&
                       element[1]->QueryFloatAttribute("z", &tmpOffset[2] ) == tinyxml2::XML_NO_ERROR)
                    {
                        glm::vec3 sceneOffset(1.0f);
						sceneOffset.x = tmpOffset[0];
                        sceneOffset.y = tmpOffset[1];
                        sceneOffset.z = tmpOffset[2];
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting scene offset to (%f, %f, %f)\n",
                                                                sceneOffset.x,
                                                                sceneOffset.y,
                                                                sceneOffset.z);
                        
						ClusterManager::instance()->setSceneOffset( sceneOffset );
                    }
                    else
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse scene offset from XML!\n");
				}
				else if( strcmp("Orientation", val[1]) == 0 )
				{
					ClusterManager::instance()->setSceneRotation(glm::mat4_cast(parseOrientationNode(element[1])));
				}
				else if( strcmp("Scale", val[1]) == 0 )
				{
					float tmpScale = 1.0f;
					if( element[1]->QueryFloatAttribute("value", &tmpScale ) == tinyxml2::XML_NO_ERROR )
                    {
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting scene scale to %f\n",
                                                                tmpScale );
                        
						ClusterManager::instance()->setSceneScale( tmpScale );
                    }
                    else
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse scene orientation from XML!\n");
				}
                
				//iterate
				element[1] = element[1]->NextSiblingElement();
			}
        }
		else if( strcmp("Node", val[0]) == 0 )
		{
			SGCTNode tmpNode;
            
			if( element[0]->Attribute( "address" ) )
				tmpNode.setAddress( element[0]->Attribute( "address" ) );
			if (element[0]->Attribute("name"))
				tmpNode.setName(element[0]->Attribute("name"));
			if( element[0]->Attribute( "ip" ) ) //backward compability with older versions of SGCT config files
				tmpNode.setAddress( element[0]->Attribute( "ip" ) );
			if( element[0]->Attribute( "port" ) )
				tmpNode.setSyncPort( element[0]->Attribute( "port" ) );
			if (element[0]->Attribute("syncPort"))
				tmpNode.setSyncPort(element[0]->Attribute("syncPort"));
			if (element[0]->Attribute("dataTransferPort"))
				tmpNode.setDataTransferPort(element[0]->Attribute("dataTransferPort"));
            
			if( element[0]->Attribute("swapLock") != NULL )
				tmpNode.setUseSwapGroups( strcmp( element[0]->Attribute("swapLock"), "true" ) == 0 ? true : false );
            
			element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();
				if( strcmp("Window", val[1]) == 0 )
				{
					sgct::SGCTWindow tmpWin( static_cast<int>(tmpNode.getNumberOfWindows()) );
                    
					if( element[1]->Attribute("name") != NULL )
						tmpWin.setName( element[1]->Attribute("name") );

					if (element[1]->Attribute("bufferBitDepth") != NULL)
						tmpWin.setColorBitDepth(getBufferColorBitDepth(element[1]->Attribute("bufferBitDepth")));

					if (element[1]->Attribute("preferBGR") != NULL)
						tmpWin.setPreferBGR(strcmp(element[1]->Attribute("preferBGR"), "true") == 0);
						
					//compability with older versions
					if (element[1]->Attribute("fullScreen") != NULL)
						tmpWin.setWindowMode(strcmp(element[1]->Attribute("fullScreen"), "true") == 0);

					if( element[1]->Attribute("fullscreen") != NULL )
						tmpWin.setWindowMode( strcmp( element[1]->Attribute("fullscreen"), "true" ) == 0 );
                    
					if( element[1]->Attribute("floating") != NULL )
						tmpWin.setFloating( strcmp( element[1]->Attribute("floating"), "true" ) == 0 );

					if (element[1]->Attribute("alwaysRender") != NULL)
						tmpWin.setRenderWhileHidden(strcmp(element[1]->Attribute("alwaysRender"), "true") == 0);

					if (element[1]->Attribute("hidden") != NULL)
						tmpWin.setVisibility(!(strcmp(element[1]->Attribute("hidden"), "true") == 0));

					if (element[1]->Attribute("dbuffered") != NULL)
						tmpWin.setDoubleBuffered(strcmp(element[1]->Attribute("dbuffered"), "true") == 0);

					float gamma = 0.0f;
					if (element[1]->QueryFloatAttribute("gamma", &gamma) == tinyxml2::XML_NO_ERROR && gamma > 0.1f)
						tmpWin.setGamma(gamma);

					float contrast = -1.0f;
					if (element[1]->QueryFloatAttribute("contrast", &contrast) == tinyxml2::XML_NO_ERROR && contrast > 0.0f)
						tmpWin.setContrast(contrast);

					float brightness = -1.0f;
					if (element[1]->QueryFloatAttribute("brightness", &brightness) == tinyxml2::XML_NO_ERROR && brightness > 0.0f)
						tmpWin.setContrast(brightness);
                    
					int tmpSamples = 0;
					//compability with older versions
					if( element[1]->QueryIntAttribute("numberOfSamples", &tmpSamples ) == tinyxml2::XML_NO_ERROR && tmpSamples <= 128)
						tmpWin.setNumberOfAASamples(tmpSamples);
					else if( element[1]->QueryIntAttribute("msaa", &tmpSamples ) == tinyxml2::XML_NO_ERROR && tmpSamples <= 128)
						tmpWin.setNumberOfAASamples(tmpSamples);
					else if (element[1]->QueryIntAttribute("MSAA", &tmpSamples) == tinyxml2::XML_NO_ERROR && tmpSamples <= 128)
						tmpWin.setNumberOfAASamples(tmpSamples);
                    
					if (element[1]->Attribute("alpha") != NULL)
						tmpWin.setAlpha(strcmp(element[1]->Attribute("alpha"), "true") == 0 ? true : false);
                    
					if( element[1]->Attribute("fxaa") != NULL )
						tmpWin.setUseFXAA( strcmp( element[1]->Attribute("fxaa"), "true" ) == 0 ? true : false );
                    
					if( element[1]->Attribute("FXAA") != NULL )
						tmpWin.setUseFXAA( strcmp( element[1]->Attribute("FXAA"), "true" ) == 0 ? true : false );
                    
					if( element[1]->Attribute("decorated") != NULL )
						tmpWin.setWindowDecoration( strcmp( element[1]->Attribute("decorated"), "true" ) == 0 ? true : false);
                    
					if( element[1]->Attribute("border") != NULL )
						tmpWin.setWindowDecoration( strcmp( element[1]->Attribute("border"), "true" ) == 0 ? true : false);
                    
					int tmpMonitorIndex = 0;
					if( element[1]->QueryIntAttribute("monitor", &tmpMonitorIndex ) == tinyxml2::XML_NO_ERROR)
						tmpWin.setFullScreenMonitorIndex( tmpMonitorIndex );
                    
					element[2] = element[1]->FirstChildElement();
					while( element[2] != NULL )
					{
						val[2] = element[2]->Value();
						int tmpWinData[2];
						memset(tmpWinData,0,4);
                        
						if( strcmp("Stereo", val[2]) == 0 )
						{
							tmpWin.setStereoMode( getStereoType( element[2]->Attribute("type") ) );
						}
						else if( strcmp("Pos", val[2]) == 0 )
						{
							if( element[2]->QueryIntAttribute("x", &tmpWinData[0] ) == tinyxml2::XML_NO_ERROR &&
                               element[2]->QueryIntAttribute("y", &tmpWinData[1] ) == tinyxml2::XML_NO_ERROR )
                                tmpWin.setWindowPosition(tmpWinData[0],tmpWinData[1]);
                            else
                                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse window position from XML!\n");
						}
						else if( strcmp("Size", val[2]) == 0 )
						{
							if( element[2]->QueryIntAttribute("x", &tmpWinData[0] ) == tinyxml2::XML_NO_ERROR &&
                               element[2]->QueryIntAttribute("y", &tmpWinData[1] ) == tinyxml2::XML_NO_ERROR )
                                tmpWin.initWindowResolution(tmpWinData[0],tmpWinData[1]);
                            else
                                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse window resolution from XML!\n");
						}
						else if( strcmp("Res", val[2]) == 0 )
						{
							if( element[2]->QueryIntAttribute("x", &tmpWinData[0] ) == tinyxml2::XML_NO_ERROR &&
                               element[2]->QueryIntAttribute("y", &tmpWinData[1] ) == tinyxml2::XML_NO_ERROR )
							{
                                tmpWin.setFramebufferResolution(tmpWinData[0],tmpWinData[1]);
								tmpWin.setFixResolution(true);
							}
                            else
                                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse frame buffer resolution from XML!\n");
						}
						else if(strcmp("Viewport", val[2]) == 0)
						{
							Viewport * vpPtr = new sgct_core::Viewport();
							vpPtr->configure(element[2]);
							tmpWin.addViewport(vpPtr);
						}//end viewport
						/*else if(strcmp("Fisheye", val[2]) == 0)
						{
							float fov;
							if( element[2]->QueryFloatAttribute("fov", &fov) == tinyxml2::XML_NO_ERROR )
								tmpWin.setFisheyeFOV( fov );
                            
							if( element[2]->Attribute("quality") != NULL )
							{
								int resolution = getFisheyeCubemapRes( std::string(element[2]->Attribute("quality")) );
								if( resolution > 0 )
									tmpWin.setCubeMapResolution( resolution );
							}
                            
                            if( element[2]->Attribute("method") != NULL )
								sgct::SGCTSettings::instance()->setFisheyeMethod(
                                                                                 strcmp( element[2]->Attribute("method"), "five_face_cube" ) == 0 ?
                                                                                 sgct::SGCTSettings::FiveFaceCube : sgct::SGCTSettings::FourFaceCube);
                            
							if (element[2]->Attribute("interpolation") != NULL)
								tmpWin.setFisheyeUseCubicInterpolation(
                                                                       strcmp( element[2]->Attribute("interpolation"), "cubic") == 0 ? true : false);
                            
							float tilt;
							if( element[2]->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR )
							{
								tmpWin.setFisheyeTilt( tilt );
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting fisheye tilt to %f degrees.\n", tilt);
							}
                            
							float diameter;
							if( element[2]->QueryFloatAttribute("diameter", &diameter) == tinyxml2::XML_NO_ERROR )
							{
								tmpWin.setDomeDiameter( diameter );
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting fisheye diameter to %f meters.\n", diameter);
							}
                            
							element[3] = element[2]->FirstChildElement();
							while( element[3] != NULL )
							{
								val[3] = element[3]->Value();
                                
								if( strcmp("Crop", val[3]) == 0 )
								{
									float tmpFArr[] = { 0.0f, 0.0f, 0.0f, 0.0f };
									float ftmp;
                                    
									if( element[3]->QueryFloatAttribute("left", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[sgct::SGCTWindow::CropLeft] = ftmp;
									if( element[3]->QueryFloatAttribute("right", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[sgct::SGCTWindow::CropRight] = ftmp;
									if( element[3]->QueryFloatAttribute("bottom", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[sgct::SGCTWindow::CropBottom] = ftmp;
									if( element[3]->QueryFloatAttribute("top", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[sgct::SGCTWindow::CropTop] = ftmp;
                                    
									tmpWin.setFisheyeCropValues(
                                                                tmpFArr[sgct::SGCTWindow::CropLeft],
                                                                tmpFArr[sgct::SGCTWindow::CropRight],
                                                                tmpFArr[sgct::SGCTWindow::CropBottom],
                                                                tmpFArr[sgct::SGCTWindow::CropTop]);
								}
								else if( strcmp("Offset", val[3]) == 0 )
								{
									float tmpFArr[] = { 0.0f, 0.0f, 0.0f };
									float ftmp;
                                    
									if( element[3]->QueryFloatAttribute("x", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[0] = ftmp;
									if( element[3]->QueryFloatAttribute("y", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[1] = ftmp;
									if( element[3]->QueryFloatAttribute("z", &ftmp) == tinyxml2::XML_NO_ERROR )
										tmpFArr[2] = ftmp;
                                    
									tmpWin.setFisheyeBaseOffset(tmpFArr[0], tmpFArr[1], tmpFArr[2]);
								}
                                
								//iterate
								element[3] = element[3]->NextSiblingElement();
							}
                            
							tmpWin.setFisheyeRendering(true);
                            
						}//end fisheye*/
                        
						//iterate
						element[2] = element[2]->NextSiblingElement();
					}
                    
					tmpNode.addWindow( tmpWin );
				}//end window
                
				//iterate
				element[1] = element[1]->NextSiblingElement();
                
			}//end while
            
			ClusterManager::instance()->addNode(tmpNode);
		}//end if node
		else if( strcmp("User", val[0]) == 0 )
		{
			SGCTUser * usrPtr;
			if (element[0]->Attribute("name") != NULL)
			{
				std::string name(element[0]->Attribute("name"));
				usrPtr = new SGCTUser(name);
				ClusterManager::instance()->addUserPtr(usrPtr);
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ReadConfig: Adding user '%s'!\n", name.c_str());
			}
			else
				usrPtr = ClusterManager::instance()->getDefaultUserPtr();

			float fTmp;
			if( element[0]->QueryFloatAttribute("eyeSeparation", &fTmp) == tinyxml2::XML_NO_ERROR )
				usrPtr->setEyeSeparation(fTmp);
            /*else -- not required
             sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse user eye separation from XML!\n");*/
            
			element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();
                
				if( strcmp("Pos", val[1]) == 0 )
				{
					float fTmp[3];
					if (element[1]->QueryFloatAttribute("x", &fTmp[0]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y", &fTmp[1]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z", &fTmp[2]) == tinyxml2::XML_NO_ERROR)
						usrPtr->setPos(fTmp);
                    else
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse user position from XML!\n");
				}
				else if( strcmp("Orientation", val[1]) == 0 )
				{
					usrPtr->setOrientation( parseOrientationNode(element[1]) );
				}
				else if (strcmp("Quaternion", val[1]) == 0)
				{
					float tmpd[4];
					if (element[1]->QueryFloatAttribute("w", &tmpd[0]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x", &tmpd[1]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y", &tmpd[2]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z", &tmpd[3]) == tinyxml2::XML_NO_ERROR)
					{
						glm::quat q(tmpd[0], tmpd[1], tmpd[2], tmpd[3]);
						usrPtr->setOrientation(q);
					}
					else
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse device orientation in XML!\n");
				}
				else if (strcmp("Matrix", val[1]) == 0)
				{
					bool transpose = true;
					if (element[1]->Attribute("transpose") != NULL)
						transpose = (strcmp(element[1]->Attribute("transpose"), "true") == 0);

					float tmpf[16];
					if (element[1]->QueryFloatAttribute("x0", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y0", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z0", &tmpf[2]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w0", &tmpf[3]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x1", &tmpf[4]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y1", &tmpf[5]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z1", &tmpf[6]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w1", &tmpf[7]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x2", &tmpf[8]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y2", &tmpf[9]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z2", &tmpf[10]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w2", &tmpf[11]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x3", &tmpf[12]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y3", &tmpf[13]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z3", &tmpf[14]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w3", &tmpf[15]) == tinyxml2::XML_NO_ERROR)
					{
						//glm & opengl uses column major order (normally row major order is used in linear algebra)
						glm::mat4 mat = glm::make_mat4(tmpf);
						if (transpose)
							mat = glm::transpose(mat);
						usrPtr->setTransform(mat);
					}
					else
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse user matrix in XML!\n");
				}
				else if( strcmp("Tracking", val[1]) == 0 )
				{
					if(	element[1]->Attribute("tracker") != NULL &&
                       element[1]->Attribute("device") != NULL )
					{
						usrPtr->setHeadTracker( element[1]->Attribute("tracker"), element[1]->Attribute("device") );
					}
					else
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse user tracking data from XML!\n");
				}
                
				//iterate
				element[1] = element[1]->NextSiblingElement();
			}
		}//end user
		else if( strcmp("Settings", val[0]) == 0 )
		{
			element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();
                
				if( strcmp("DepthBufferTexture", val[1]) == 0 )
				{
					if (element[1]->Attribute("value") != NULL)
						sgct::SGCTSettings::instance()->setUseDepthTexture(strcmp(element[1]->Attribute("value"), "true") == 0 ? true : false);
				}
				else if (strcmp("NormalTexture", val[1]) == 0)
				{
					if (element[1]->Attribute("value") != NULL)
						sgct::SGCTSettings::instance()->setUseNormalTexture(strcmp(element[1]->Attribute("value"), "true") == 0 ? true : false);
				}
				else if (strcmp("PositionTexture", val[1]) == 0)
				{
					if (element[1]->Attribute("value") != NULL)
						sgct::SGCTSettings::instance()->setUsePositionTexture(strcmp(element[1]->Attribute("value"), "true") == 0 ? true : false);
				}
				else if (strcmp("PBO", val[1]) == 0)
				{
					if (element[1]->Attribute("value") != NULL)
						sgct::SGCTSettings::instance()->setUsePBO(strcmp(element[1]->Attribute("value"), "true") == 0 ? true : false);
				}
				else if (strcmp("Precision", val[1]) == 0)
				{
					int fprec = 0;
					if (element[1]->QueryIntAttribute("float", &fprec) == tinyxml2::XML_NO_ERROR)
					{
						if (fprec == 16)
							sgct::SGCTSettings::instance()->setBufferFloatPrecision(sgct::SGCTSettings::Float_16Bit);
						else if (fprec == 32)
							sgct::SGCTSettings::instance()->setBufferFloatPrecision(sgct::SGCTSettings::Float_32Bit);
						else
							sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "ReadConfig: Invalid float precition value (%d)! Must be 16 or 32.\n", fprec);
					}
					else
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "ReadConfig: Invalid float precition value! Must be 16 or 32.\n");
				}
				else if( strcmp("Display", val[1]) == 0 )
				{
					int tmpInterval = 0;
					if( element[1]->QueryIntAttribute("swapInterval", &tmpInterval) == tinyxml2::XML_NO_ERROR )
					{
						sgct::SGCTSettings::instance()->setSwapInterval( tmpInterval );
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ReadConfig: Display swap interval is set to %d.\n", tmpInterval);
					}
                    
					int rate = 0;
					if( element[1]->QueryIntAttribute("refreshRate", &rate) == tinyxml2::XML_NO_ERROR )
					{
						sgct::SGCTSettings::instance()->setRefreshRateHint( rate );
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ReadConfig: Display refresh rate hint is set to %d Hz.\n", rate);
					}

					if (element[1]->Attribute("tryMaintainAspectRatio") != NULL)
						sgct::SGCTSettings::instance()->setTryMaintainAspectRatio(strcmp(element[1]->Attribute("tryMaintainAspectRatio"), "true") == 0 ? true : false);
				}
				else if( strcmp("OSDText", val[1]) == 0 )
				{
					float x = 0.0f;
					float y = 0.0f;
                    
					if( element[1]->Attribute("name") != NULL )
					{
						sgct::SGCTSettings::instance()->setOSDTextFontName( element[1]->Attribute("name") );
						sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                "ReadConfig: Setting font name to %s\n", element[1]->Attribute("name") );
					}
                    
					if( element[1]->Attribute("path") != NULL )
					{
						sgct::SGCTSettings::instance()->setOSDTextFontPath( element[1]->Attribute("path") );
						sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                "ReadConfig: Setting font path to %s\n", element[1]->Attribute("path") );
					}
                    
					if( element[1]->Attribute("size") != NULL )
					{
						unsigned int tmpi;
						if( element[1]->QueryUnsignedAttribute("size", &tmpi) == tinyxml2::XML_NO_ERROR && tmpi > 0)
						{
							sgct::SGCTSettings::instance()->setOSDTextFontSize( tmpi );
							sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                    "ReadConfig: Setting font size to %u\n", tmpi );
						}
						else
							sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "ReadConfig: Font size not specified. Setting to default size=10!\n");
					}
                    
					if( element[1]->QueryFloatAttribute("xOffset", &x) == tinyxml2::XML_NO_ERROR )
                    {
						sgct::SGCTSettings::instance()->setOSDTextXOffset( x );
						sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                "ReadConfig: Setting font x offset to %f\n", x );
                    }
                    
					if( element[1]->QueryFloatAttribute("yOffset", &y) == tinyxml2::XML_NO_ERROR )
					{
						sgct::SGCTSettings::instance()->setOSDTextYOffset( y );
						sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                "ReadConfig: Setting font y offset to %f\n", y );
                    }
				}
				else if( strcmp("FXAA", val[1]) == 0 )
				{
					float offset = 0.0f;
					if( element[1]->QueryFloatAttribute("offset", &offset) == tinyxml2::XML_NO_ERROR)
					{
						sgct::SGCTSettings::instance()->setFXAASubPixOffset( offset );
						sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                "ReadConfig: Setting FXAA sub-pixel offset to %f\n", offset );
					}
                    
					float trim = 0.0f;
					if( element[1]->QueryFloatAttribute("trim", &trim) == tinyxml2::XML_NO_ERROR )
					{
						if(trim > 0.0f)
						{
							sgct::SGCTSettings::instance()->setFXAASubPixTrim( 1.0f/trim );
							sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                    "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 1.0f/trim );
						}
						else
						{
							sgct::SGCTSettings::instance()->setFXAASubPixTrim( 0.0f );
							sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
                                                                    "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 0.0f );
						}
					}
				}
                
				//iterate
				element[1] = element[1]->NextSiblingElement();
			}
		}//end settings
		else if( strcmp("Capture", val[0]) == 0 )
		{
			if( element[0]->Attribute("path") != NULL )
			{
			    sgct::SGCTSettings::instance()->setCapturePath( element[0]->Attribute("path"), sgct::SGCTSettings::Mono );
				sgct::SGCTSettings::instance()->setCapturePath( element[0]->Attribute("path"), sgct::SGCTSettings::LeftStereo );
				sgct::SGCTSettings::instance()->setCapturePath( element[0]->Attribute("path"), sgct::SGCTSettings::RightStereo );
            }
			if( element[0]->Attribute("monoPath") != NULL )
			{
			    sgct::SGCTSettings::instance()->setCapturePath( element[0]->Attribute("monoPath"), sgct::SGCTSettings::Mono );
            }
			if( element[0]->Attribute("leftPath") != NULL )
			{
			    sgct::SGCTSettings::instance()->setCapturePath( element[0]->Attribute("leftPath"), sgct::SGCTSettings::LeftStereo );
            }
			if( element[0]->Attribute("rightPath") != NULL )
			{
			    sgct::SGCTSettings::instance()->setCapturePath( element[0]->Attribute("rightPath"), sgct::SGCTSettings::RightStereo );
            }
            
            if( element[0]->Attribute("format") != NULL )
			{
			    sgct::SGCTSettings::instance()->setCaptureFormat( element[0]->Attribute("format") );
            }
        }
		else if( strcmp("Tracker", val[0]) == 0 && element[0]->Attribute("name") != NULL )
		{
			ClusterManager::instance()->getTrackingManagerPtr()->addTracker( std::string(element[0]->Attribute("name")) );
            
			element[1] = element[0]->FirstChildElement();
			while( element[1] != NULL )
			{
				val[1] = element[1]->Value();
                
				if( strcmp("Device", val[1]) == 0 && element[1]->Attribute("name") != NULL)
				{
					ClusterManager::instance()->getTrackingManagerPtr()->addDeviceToCurrentTracker( std::string(element[1]->Attribute("name")) );
                    
					element[2] = element[1]->FirstChildElement();
                    
					while( element[2] != NULL )
					{
						val[2] = element[2]->Value();
						unsigned int tmpUI = 0;
						int tmpi = -1;
                        
						if( strcmp("Sensor", val[2]) == 0 )
						{
							if( element[2]->Attribute("vrpnAddress") != NULL &&
                               element[2]->QueryIntAttribute("id", &tmpi) == tinyxml2::XML_NO_ERROR )
							{
                                ClusterManager::instance()->getTrackingManagerPtr()->addSensorToCurrentDevice(
                                                                                                              element[2]->Attribute("vrpnAddress"), tmpi);
							}
						}
						else if( strcmp("Buttons", val[2]) == 0 )
						{
							if(element[2]->Attribute("vrpnAddress") != NULL &&
                               element[2]->QueryUnsignedAttribute("count", &tmpUI) == tinyxml2::XML_NO_ERROR )
                            {
                                ClusterManager::instance()->getTrackingManagerPtr()->addButtonsToCurrentDevice(
                                                                                                               element[2]->Attribute("vrpnAddress"), tmpUI);
                            }
                            
						}
						else if( strcmp("Axes", val[2]) == 0 )
						{
							if(element[2]->Attribute("vrpnAddress") != NULL &&
                               element[2]->QueryUnsignedAttribute("count", &tmpUI) == tinyxml2::XML_NO_ERROR )
                            {
                                ClusterManager::instance()->getTrackingManagerPtr()->addAnalogsToCurrentDevice(
                                                                                                               element[2]->Attribute("vrpnAddress"), tmpUI);
                            }
						}
						else if( strcmp("Offset", val[2]) == 0 )
						{
							float tmpf[3];
							if (element[2]->QueryFloatAttribute("x", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("y", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("z", &tmpf[2]) == tinyxml2::XML_NO_ERROR)
								ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->getLastDevicePtr()->
                                setOffset( tmpf[0], tmpf[1], tmpf[2] );
							else
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse device offset in XML!\n");
						}
						else if( strcmp("Orientation", val[2]) == 0 )
						{
							ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->getLastDevicePtr()->
                                setOrientation( parseOrientationNode( element[2] ) );
						}
						else if (strcmp("Quaternion", val[2]) == 0)
						{
							float tmpf[4];
							if (element[2]->QueryFloatAttribute("w", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("x", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("y", &tmpf[2]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("z", &tmpf[3]) == tinyxml2::XML_NO_ERROR)
								ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->getLastDevicePtr()->
                                setOrientation(tmpf[0], tmpf[1], tmpf[2], tmpf[3]);
							else
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse device orientation in XML!\n");
						}
						else if (strcmp("Matrix", val[2]) == 0)
						{
							bool transpose = true;
							if (element[2]->Attribute("transpose") != NULL)
								transpose = (strcmp(element[2]->Attribute("transpose"), "true") == 0);
							
							float tmpf[16];
							if (element[2]->QueryFloatAttribute("x0", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("y0", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("z0", &tmpf[2]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("w0", &tmpf[3]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("x1", &tmpf[4]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("y1", &tmpf[5]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("z1", &tmpf[6]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("w1", &tmpf[7]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("x2", &tmpf[8]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("y2", &tmpf[9]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("z2", &tmpf[10]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("w2", &tmpf[11]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("x3", &tmpf[12]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("y3", &tmpf[13]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("z3", &tmpf[14]) == tinyxml2::XML_NO_ERROR &&
								element[2]->QueryFloatAttribute("w3", &tmpf[15]) == tinyxml2::XML_NO_ERROR)
							{
								//glm & opengl uses column major order (normally row major order is used in linear algebra)
								glm::mat4 mat = glm::make_mat4( tmpf );
								if (transpose)
									mat = glm::transpose(mat);
								ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->getLastDevicePtr()->setTransform( mat );
							}
							else
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse device matrix in XML!\n");
						}
                        
						//iterate
						element[2] = element[2]->NextSiblingElement();
					}
                    
				}
				else if( strcmp("Offset", val[1]) == 0 )
				{
					float tmpf[3];
					if (element[1]->QueryFloatAttribute("x", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z", &tmpf[2]) == tinyxml2::XML_NO_ERROR)
						ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->setOffset(tmpf[0], tmpf[1], tmpf[2]);
                    else
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse tracker offset in XML!\n");
				}
				else if( strcmp("Orientation", val[1]) == 0 )
				{
					ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->setOrientation( parseOrientationNode( element[1] ) );
				}
				else if (strcmp("Quaternion", val[1]) == 0)
				{
					float tmpf[4];
					if (element[1]->QueryFloatAttribute("w", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y", &tmpf[2]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z", &tmpf[3]) == tinyxml2::XML_NO_ERROR)
						ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->setOrientation(tmpf[0], tmpf[1], tmpf[2], tmpf[3]);
					else
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse tracker orientation quaternion in XML!\n");
				}
				else if( strcmp("Scale", val[1]) == 0 )
				{
					double scaleVal;
					if( element[1]->QueryDoubleAttribute("value", &scaleVal) == tinyxml2::XML_NO_ERROR )
						ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->setScale( scaleVal );
                    else
                        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse tracker scale in XML!\n");
				}
				else if (strcmp("Matrix", val[1]) == 0)
				{
					bool transpose = true;
					if (element[1]->Attribute("transpose") != NULL)
						transpose = (strcmp(element[1]->Attribute("transpose"), "true") == 0);

					float tmpf[16];
					if (element[1]->QueryFloatAttribute("x0", &tmpf[0]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y0", &tmpf[1]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z0", &tmpf[2]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w0", &tmpf[3]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x1", &tmpf[4]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y1", &tmpf[5]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z1", &tmpf[6]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w1", &tmpf[7]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x2", &tmpf[8]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y2", &tmpf[9]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z2", &tmpf[10]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w2", &tmpf[11]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("x3", &tmpf[12]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("y3", &tmpf[13]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("z3", &tmpf[14]) == tinyxml2::XML_NO_ERROR &&
						element[1]->QueryFloatAttribute("w3", &tmpf[15]) == tinyxml2::XML_NO_ERROR)
					{
						//glm & opengl uses column major order (normally row major order is used in linear algebra)
						glm::mat4 mat = glm::make_mat4(tmpf);
						if (transpose)
							mat = glm::transpose(mat);
						ClusterManager::instance()->getTrackingManagerPtr()->getLastTrackerPtr()->setTransform(mat);
					}
					else
						sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ReadConfig: Failed to parse tracker matrix in XML!\n");
				}
                
				//iterate
				element[1] = element[1]->NextSiblingElement();
			}
		}// end tracking part
        
		//iterate
		element[0] = element[0]->NextSiblingElement();
	}

	return true;
}

sgct::SGCTWindow::StereoMode sgct_core::ReadConfig::getStereoType( std::string type )
{
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    
	if( strcmp( type.c_str(), "none" ) == 0 || strcmp( type.c_str(), "no_stereo" ) == 0  )
		return sgct::SGCTWindow::No_Stereo;
	else if( strcmp( type.c_str(), "active" ) == 0 || strcmp( type.c_str(), "quadbuffer" ) == 0 )
		return sgct::SGCTWindow::Active_Stereo;
	else if( strcmp( type.c_str(), "checkerboard" ) == 0 )
		return sgct::SGCTWindow::Checkerboard_Stereo;
	else if( strcmp( type.c_str(), "checkerboard_inverted" ) == 0 )
		return sgct::SGCTWindow::Checkerboard_Inverted_Stereo;
	else if( strcmp( type.c_str(), "anaglyph_red_cyan" ) == 0 )
		return sgct::SGCTWindow::Anaglyph_Red_Cyan_Stereo;
	else if( strcmp( type.c_str(), "anaglyph_amber_blue" ) == 0 )
		return sgct::SGCTWindow::Anaglyph_Amber_Blue_Stereo;
	else if( strcmp( type.c_str(), "anaglyph_wimmer" ) == 0 )
		return sgct::SGCTWindow::Anaglyph_Red_Cyan_Wimmer_Stereo;
	else if( strcmp( type.c_str(), "vertical_interlaced" ) == 0 )
		return sgct::SGCTWindow::Vertical_Interlaced_Stereo;
	else if( strcmp( type.c_str(), "vertical_interlaced_inverted" ) == 0 )
		return sgct::SGCTWindow::Vertical_Interlaced_Inverted_Stereo;
	else if( strcmp( type.c_str(), "test" ) == 0 || strcmp( type.c_str(), "dummy" ) == 0 )
		return sgct::SGCTWindow::Dummy_Stereo;
	else if( strcmp( type.c_str(), "side_by_side" ) == 0 )
		return sgct::SGCTWindow::Side_By_Side_Stereo;
	else if( strcmp( type.c_str(), "side_by_side_inverted" ) == 0 )
		return sgct::SGCTWindow::Side_By_Side_Inverted_Stereo;
	else if( strcmp( type.c_str(), "top_bottom" ) == 0 )
		return sgct::SGCTWindow::Top_Bottom_Stereo;
	else if( strcmp( type.c_str(), "top_bottom_inverted" ) == 0 )
		return sgct::SGCTWindow::Top_Bottom_Inverted_Stereo;
    
	//if match not found
	return sgct::SGCTWindow::No_Stereo;
}

sgct::SGCTWindow::ColorBitDepth sgct_core::ReadConfig::getBufferColorBitDepth(std::string type)
{
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

	if (strcmp(type.c_str(), "8") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth8;
	else if (strcmp(type.c_str(), "16") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth16;
	
	else if (strcmp(type.c_str(), "16f") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth16Float;
	else if (strcmp(type.c_str(), "32f") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth32Float;
	
	else if (strcmp(type.c_str(), "16i") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth16Int;
	else if (strcmp(type.c_str(), "32i") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth32Int;

	else if (strcmp(type.c_str(), "16ui") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth16UInt;
	else if (strcmp(type.c_str(), "32ui") == 0)
		return sgct::SGCTWindow::BufferColorBitDepth32UInt;

	//default
	return sgct::SGCTWindow::BufferColorBitDepth8;
}

glm::quat sgct_core::ReadConfig::parseOrientationNode(tinyxml2::XMLElement* element)
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float tmpf;

	bool eulerMode = false;
	bool quatMode = false;

	glm::quat quat;

	if (element->QueryFloatAttribute("w", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		quat.w = tmpf;
		quatMode = true;
	}

	if (element->QueryFloatAttribute("y", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		y = tmpf;
		eulerMode = true;
	}

	if (element->QueryFloatAttribute("yaw", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		y = tmpf;
	}

	if (element->QueryFloatAttribute("heading", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		y = tmpf;
	}

	if (element->QueryFloatAttribute("azimuth", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		y = -tmpf;
	}

	if (element->QueryFloatAttribute("x", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		x = tmpf;
		eulerMode = true;
	}

	if (element->QueryFloatAttribute("pitch", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		x = tmpf;
	}

	if (element->QueryFloatAttribute("elevation", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		x = tmpf;
	}

	if (element->QueryFloatAttribute("z", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		z = tmpf;
		eulerMode = true;
	}

	if (element->QueryFloatAttribute("roll", &tmpf) == tinyxml2::XML_NO_ERROR)
	{
		z = -tmpf;
	}

	if (quatMode)
	{
		quat.x = x;
		quat.y = y;
		quat.z = z;
	}
	else
	{
		if (eulerMode)
		{
			quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
			quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
			quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else
		{
			quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
			quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
			quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.0f, 0.0f, 1.0f));
		}
	}

	return quat;
}