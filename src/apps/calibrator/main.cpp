#include "Dome.h"

sgct::Engine * gEngine;

void draw();
void initGL();
void preSync();
void encode();
void decode();
void keyCallback(int key, int action);

void drawGeoCorrPatt();
void drawColCorrPatt();
void drawCube();
void loadData();
void drawTexturedObject();

Dome * mDome;

sgct::SharedShort displayState(0);
sgct::SharedShort colorState(0);
sgct::SharedBool showGeoCorrectionPattern(true);
sgct::SharedBool showBlendZones(false);
sgct::SharedBool showChannelZones(false);
sgct::SharedBool takeScreenShot(false);
sgct::SharedBool wireframe(false);

const short lastState = 7;
bool useShader = true;
bool isTiltSet = false;
bool useDisplayLists = false;
double tilt = 0.0;
std::vector<glm::vec3> colors;
std::vector<std::pair<std::string, unsigned int>> textures;

int main( int argc, char* argv[] )
{

	// Allocate
	gEngine = new sgct::Engine( argc, argv );

	//parse arguments
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-tex") == 0 && argc >(i + 1))
		{
			std::pair<std::string, unsigned int> tmpPair;
			tmpPair.first.assign(argv[i + 1]);
			tmpPair.second = GL_FALSE;
			textures.push_back(tmpPair);
			
			sgct::MessageHandler::instance()->print("Adding texture: %s\n", argv[i + 1]);
		}
		else if (strcmp(argv[i], "-tilt") == 0 && argc > (i + 1))
		{
			tilt = atof(argv[i + 1]);
			isTiltSet = true;

			sgct::MessageHandler::instance()->print("Setting tilt to: %f\n", tilt);
		}
		else if (strcmp(argv[i], "--use-display-lists") == 0)
		{
			useDisplayLists = true;
			sgct::MessageHandler::instance()->print("Display lists will be used in legacy pipeline.\n");
		}
	}

	if( useDisplayLists )
		sgct_core::ClusterManager::instance()->setMeshImplementation( sgct_core::ClusterManager::DISPLAY_LIST );
	else
		sgct_core::ClusterManager::instance()->setMeshImplementation( sgct_core::ClusterManager::VBO );
    
    sgct::SGCTSettings::instance()->setCaptureFormat("tga");

	// Bind your functions
	gEngine->setDrawFunction( draw );
	gEngine->setInitOGLFunction( initGL );
	gEngine->setPreSyncFunction( preSync );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	sgct::SharedData::instance()->setEncodeFunction(encode);
	sgct::SharedData::instance()->setDecodeFunction(decode);

	// Init the engine
	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	// Main loop
	gEngine->render();

	// Clean up (de-allocate)
    delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void draw()
{
	switch( displayState.getVal() )
	{
	case 0:
	default:
		drawGeoCorrPatt();
		break;

	case 1:
		drawColCorrPatt();
		break;

	case 2:
		drawColCorrPatt();
		break;

	case 3:
		drawColCorrPatt();
		break;

	case 4:
		drawColCorrPatt();
		break;

	case 5:
		drawColCorrPatt();
		break;

	case 6:
		drawCube();
		break;

	case 7:
		drawTexturedObject();
		break;
	}
}

void initGL()
{
	colors.push_back( glm::vec3(1.00f, 1.00f, 1.00f) ); //white
	colors.push_back( glm::vec3(0.25f, 0.25f, 0.25f) ); //25% gray
	colors.push_back( glm::vec3(0.50f, 0.50f, 0.50f) ); //50% gray
	colors.push_back( glm::vec3(0.75f, 0.75f, 0.75f) ); //75% gray
	colors.push_back( glm::vec3(1.00f, 0.00f, 0.00f) ); //red
	colors.push_back( glm::vec3(1.00f, 0.50f, 0.00f) ); //orange
	colors.push_back( glm::vec3(1.00f, 1.00f, 0.00f) ); //yellow
	colors.push_back( glm::vec3(0.50f, 1.00f, 0.00f) ); //yellow-green
	colors.push_back( glm::vec3(0.00f, 1.00f, 0.00f) ); //green
	colors.push_back( glm::vec3(0.00f, 1.00f, 0.50f) ); //green-cyan
	colors.push_back( glm::vec3(0.00f, 1.00f, 1.00f) ); //cyan
	colors.push_back( glm::vec3(0.00f, 0.50f, 1.00f) ); //cyan-blue
	colors.push_back( glm::vec3(0.00f, 0.00f, 1.00f) ); //blue
	colors.push_back( glm::vec3(0.50f, 0.00f, 1.00f) ); //blue-magenta
	colors.push_back( glm::vec3(1.00f, 0.00f, 1.00f) ); //magenta
	colors.push_back( glm::vec3(1.00f, 0.00f, 0.50f) ); //magenta-red
	colors.push_back( glm::vec3(1.00f, 0.00f, 0.00f) ); //red
	
	if (isTiltSet)
		mDome = new Dome(7.4f, static_cast<float>(tilt), FISHEYE);
	else
		mDome = new Dome(7.4f, 26.7f, FISHEYE);
	mDome->generateDisplayList();

	sgct::TextureManager::instance()->setAnisotropicFilterSize(4.0f);
	//sgct::TextureManager::instance()->setCompression(sgct::TextureManager::Generic);
	for (std::size_t i = 0; i < textures.size(); i++)
		sgct::TextureManager::instance()->loadUnManagedTexture(
		textures[i].second, textures[i].first, true, 4);
	
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_COLOR_MATERIAL);
	//glEnable(GL_NORMALIZE);
}

void preSync()
{
	//set the time only on the master
	if( gEngine->isMaster() )
	{
		if( takeScreenShot.getVal() )
		{
			takeScreenShot.setVal(false);
			gEngine->takeScreenshot();
		}
	}

	gEngine->setWireframe(wireframe.getVal());
}

void encode()
{
	sgct::SharedData::instance()->writeShort( &displayState );
	sgct::SharedData::instance()->writeShort( &colorState );
	sgct::SharedData::instance()->writeBool( &showGeoCorrectionPattern );
	sgct::SharedData::instance()->writeBool( &showBlendZones );
	sgct::SharedData::instance()->writeBool( &showChannelZones );
	sgct::SharedData::instance()->writeBool( &takeScreenShot );
	sgct::SharedData::instance()->writeBool( &wireframe );
}

void decode()
{
	sgct::SharedData::instance()->readShort( &displayState );
	sgct::SharedData::instance()->readShort( &colorState );
	sgct::SharedData::instance()->readBool( &showGeoCorrectionPattern );
	sgct::SharedData::instance()->readBool( &showBlendZones );
	sgct::SharedData::instance()->readBool( &showChannelZones );
	sgct::SharedData::instance()->readBool( &takeScreenShot );
	sgct::SharedData::instance()->readBool( &wireframe );
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case SGCT_KEY_LEFT:
			if(action == SGCT_PRESS)
            {
				if( displayState.getVal() > 0 )
					displayState.setVal(displayState.getVal() - 1);
				else
					displayState.setVal( lastState );
            }
			break;

		case SGCT_KEY_RIGHT:
			if(action == SGCT_PRESS)
			{
				if( displayState.getVal() < lastState )
					displayState.setVal(displayState.getVal() + 1);
				else
					displayState.setVal(0);
			}
			break;

		case SGCT_KEY_DOWN:
			if(action == SGCT_PRESS)
            {
				if( colorState.getVal() > 0 )
					colorState.setVal( colorState.getVal() - 1 );
				else
					colorState.setVal( static_cast<short>(colors.size()-1) );
            }
			break;

		case SGCT_KEY_UP:
			if(action == SGCT_PRESS)
			{
				if( colorState.getVal() < static_cast<short>(colors.size()-1) )
					colorState.setVal( colorState.getVal() + 1 );
				else
					colorState.setVal(0);
			}
			break;

		case SGCT_KEY_B:
			if(action == SGCT_PRESS)
				showBlendZones.toggle();
			break;

		case SGCT_KEY_C:
			if(action == SGCT_PRESS)
				showChannelZones.toggle();
			break;

		case SGCT_KEY_G:
			if(action == SGCT_PRESS)
				showGeoCorrectionPattern.toggle();
			break;

		case SGCT_KEY_P:
			if(action == SGCT_PRESS)
				takeScreenShot.setVal(true);
			break;

		case SGCT_KEY_W:
			if (action == SGCT_PRESS)
				wireframe.toggle();
			break;
		}
	}
}

void drawGeoCorrPatt()
{
	glDepthMask(GL_FALSE);

	if( showGeoCorrectionPattern.getVal() )
		mDome->drawGeoCorrPattern();

	if( showBlendZones.getVal() )
		mDome->drawBlendZones();

	if( showChannelZones.getVal() )
		mDome->drawChannelZones();

	glDepthMask(GL_TRUE);
}

void drawColCorrPatt()
{
	glDepthMask(GL_FALSE);
	
	mDome->drawColCorrPattern( &colors[ colorState.getVal() ],
		   static_cast<int>((displayState.getVal()-1)) %5);

	glDepthMask(GL_TRUE);
}

void drawCube()
{
	/*vpr::System::usleep( mSharedData->delay );
	
	glPushMatrix();
	glTranslatef(offset[0],offset[1],offset[2]);

	if( mSharedData->spinningCube )
	{
		static float angle = 0.0f;
		angle += static_cast<float>(mSharedData->dt * mSharedData->speed);
		glRotatef( angle, 0.0f, 1.0f, 0.0f );
	}

	glDepthMask(GL_FALSE);

	if( mSharedData->bufferTest )
	{
		static unsigned int frames = 0;
		
		if( frames % 2 == 0 )
			glColor4f(1.0f, 0.0f, 0.0f, 1.0);
		else
			glColor4f(0.0f, 1.0f, 0.0f, 1.0);

		if( frames > 10000 )
			frames = 0;
		else
			frames++;
	}
	else
		glColor4fv(colors[mSharedData->colorState].col);
	glLineWidth(2);

	float size;
	switch(calibrationType)
	{
	case CYLINDER:
		size = 5.0f;
		break;

	case DOME:
		size = mDome->getRadius() * 2.0f;
		break;

	default:
		size = 5.0f;
		break;
	}

	glBegin(GL_LINES);
		//X-lines
		
		glVertex3f( -size*0.5f, size*0.5f, size*0.5f );
		glVertex3f( size*0.5f, size*0.5f, size*0.5f );

		glVertex3f( -size*0.5f, size*0.5f, -size*0.5f );
		glVertex3f( size*0.5f, size*0.5f, -size*0.5f );

		glVertex3f( -size*0.5f, -size*0.5f, -size*0.5f );
		glVertex3f( size*0.5f, -size*0.5f, -size*0.5f );

		glVertex3f( -size*0.5f, -size*0.5f, size*0.5f );
		glVertex3f( size*0.5f, -size*0.5f, size*0.5f );

		//Y-lines
	    glVertex3f( -size*0.5f, -size*0.5f, -size*0.5f );
		glVertex3f( -size*0.5f, size*0.5f, -size*0.5f );

		glVertex3f( size*0.5f, -size*0.5f, -size*0.5f );
		glVertex3f( size*0.5f, size*0.5f, -size*0.5f );

		glVertex3f( -size*0.5f, -size*0.5f, size*0.5f );
		glVertex3f( -size*0.5f, size*0.5f, size*0.5f );

		glVertex3f( size*0.5f, -size*0.5f, size*0.5f );
		glVertex3f( size*0.5f, size*0.5f, size*0.5f );

		//Z-lines
		glVertex3f( size*0.5f, size*0.5f, -size*0.5f );
		glVertex3f( size*0.5f, size*0.5f, size*0.5f );

		glVertex3f( -size*0.5f, size*0.5f, -size*0.5f );
		glVertex3f( -size*0.5f, size*0.5f, size*0.5f );

		glVertex3f( size*0.5f, -size*0.5f, -size*0.5f );
		glVertex3f( size*0.5f, -size*0.5f, size*0.5f );

		glVertex3f( -size*0.5f, -size*0.5f, -size*0.5f );
		glVertex3f( -size*0.5f, -size*0.5f, size*0.5f );
	glEnd();

	glDepthMask(GL_TRUE);
	glPopMatrix();

	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
	freetype::print(font, 400, 400, "FPS: %.3f\nSpeed: %.0fx\nDelay: %d us", fps, mSharedData->speed, (unsigned int)mSharedData->delay);

	*/
}

void loadData()
{
	/*//todo: fixa kompression från xml
	for(unsigned int i=0;i<texFilenames.size();i++)
	  tex.loadTexure(texFilenames[i].c_str(), true, false);

	blender = Shader::create();
    const char * shaderName = "textureblend";
    useShader = blender->set( shaderName );

    if( useShader )
	{
		if ( blender->p == 0 )
			fprintf( stderr, "Shader object '%s' not created!\n", shaderName);
		else
		{
			texLoc1 = -1;
			texLoc2 = -1;
			blenderLoc = -1;
			
			texLoc1 = glGetUniformLocation( blender->p, "tex1" );
			texLoc2 = glGetUniformLocation( blender->p, "tex2" );
			blenderLoc = glGetUniformLocation( blender->p, "blender" );
			
			fprintf( stderr, "Shader object '%s' created\n", shaderName);
		}
	 }*/
}

void drawTexturedObject()
{
	if (textures.size() > 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[0].second);
		glEnable(GL_TEXTURE_2D);

		/*glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textures[0].second);
		glEnable(GL_TEXTURE_2D);*/

		mDome->drawTexturedSphere();
		
		glDisable(GL_TEXTURE_2D);
		//glActiveTexture(GL_TEXTURE0);
		//glDisable(GL_TEXTURE_2D);
	}
	
	
	/*if(tex.getStoredTextureCount() == 0)
		return;
	
	if( useShader )
    {
		float crossFadeTime = 0.6f;
		float blendVal = (mSharedData->animationTime - (10.0f-crossFadeTime))/(10.0f - (10.0f-crossFadeTime));

	    if( blendVal < 0.0f)
			blendVal = 0.0f;
	   
	    glUseProgram( blender->p );
		glUniform1i( texLoc1, 0 );
		glUniform1i( texLoc2, 1 );
		glUniform1f( blenderLoc, blendVal );
    }
	
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, tex.getTexID(mSharedData->frameCount) );
	glEnable( GL_TEXTURE_2D );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, tex.getTexID((mSharedData->frameCount+1)%tex.getStoredTextureCount()) );
	glEnable( GL_TEXTURE_2D );
	
	if( calibrationType == DOME )
	{
		mDome->setTiltOffset( mSharedData->tiltOffset );
		mDome->drawTexturedSphere();
	}

	glActiveTexture( GL_TEXTURE1 );
	glDisable( GL_TEXTURE_2D );
	glActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );	

	if( useShader )
	   glUseProgram( 0 );

	*/
}
