#include "sgct.h"

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/MatrixTransform>
#include <osg/ComputeBoundsVisitor>
#include <osgUtil/Optimizer>
#include <glm/gtx/euler_angles.hpp>

sgct::Engine * gEngine;

//Not using ref pointers enables
//more controlled termination
//and prevents segfault on Linux
osgViewer::Viewer * mViewer;
osg::ref_ptr<osg::Group>			mRootNode;
osg::ref_ptr<osg::MatrixTransform>	mSceneTrans;
osg::ref_ptr<osg::MatrixTransform>	mSceneScale;
osg::ref_ptr<osg::Node>				mModel;

//callbacks
void myInitOGLFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myDrawFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int action);
void mouseButtonCallback(int button, int action);

//other functions
void initOSG();
void setupLightSource();

//variables to share across cluster
double dt = 0.0;
double animateAngle = 0.0;
glm::mat4 xform(1.0f);
bool wireframe = false;
bool info = false;
bool stats = false;
bool takeScreenshot = false;
bool light = true;
bool culling = true;
float scale = 0.00002f;

//other var
bool animate = false;
glm::vec3 view(0.0f, 0.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 position(0.00f, 1.60f, -1.170f);
bool arrowButtonStatus[6];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT, UPWARD, DOWNWARD };
bool mouseButtonStatus[3];
bool modifierKey = false;
enum mouseButtons { LEFT_MB = 0, MIDDLE_MB, RIGHT_MB };
enum axes { X = 0, Y, Z};
float rotationSpeed = 0.003f;
float navigation_speed = 0.1f;
int mouseDiff[] = { 0, 0 };
/* Stores the positions that will be compared to measure the difference. */
int mouseXPos[] = { 0, 0 };
int mouseYPos[] = { 0, 0 };

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );

	for(int i=0; i<6; i++)
		arrowButtonStatus[i] = false;

	for(int i=0; i<3; i++)
		mouseButtonStatus[i] = false;

	if( !gEngine->init(sgct::Engine::OSG_Encapsulation_Mode) )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::Instance()->setEncodeFunction( myEncodeFun );
	sgct::SharedData::Instance()->setDecodeFunction( myDecodeFun );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myInitOGLFun()
{
	initOSG();

	osg::ref_ptr<osg::MatrixTransform> mModelTrans;

	mSceneTrans = new osg::MatrixTransform();
	mModelTrans  = new osg::MatrixTransform();
	mSceneScale = new osg::MatrixTransform();

	//rotate osg coordinate system to match sgct
	mSceneScale->setMatrix( osg::Matrix::scale(scale, scale, scale) );
	mModelTrans->setMatrix(osg::Matrix::rotate(glm::radians(-90.0f),
                                            1.0f, 0.0f, 0.0f));

	mRootNode->getOrCreateStateSet()->setMode( GL_RESCALE_NORMAL, osg::StateAttribute::ON );

	mRootNode->addChild( mSceneTrans.get() );
	mSceneTrans->addChild( mSceneScale.get() );
	mSceneScale->addChild( mModelTrans.get() );

	sgct::MessageHandler::Instance()->print("Loading model 'iss_all_maps_no_opacity.ive'...\n");
	mModel = osgDB::readNodeFile("iss_all_maps_no_opacity.ive");

	if ( mModel.valid() )
	{
		sgct::MessageHandler::Instance()->print("Model loaded successfully!\n");
		mModelTrans->addChild(mModel.get());

		//get the bounding box
		osg::ComputeBoundsVisitor cbv;
		osg::BoundingBox &bb(cbv.getBoundingBox());
		mModel->accept( cbv );
			
		osg::Vec3f tmpVec;
		tmpVec = bb.center();

		//translate model center to origin
		//mModelTrans->postMult(osg::Matrix::translate( -tmpVec[0], -tmpVec[1], -tmpVec[2] ) );

		sgct::MessageHandler::Instance()->print("Model bounding sphere center:\tx=%f\ty=%f\tz=%f\n", tmpVec[0], tmpVec[1], tmpVec[2] );
		sgct::MessageHandler::Instance()->print("Model bounding sphere radius:\t%f\n", bb.radius() );
	}
	else
		sgct::MessageHandler::Instance()->print("Failed to read model!\n");

	setupLightSource();

	//optimize scenegraph
	osgUtil::Optimizer optimizer;
	optimizer.optimize(mRootNode.get());
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		dt = gEngine->getDt();

		if( animate )
			animateAngle += dt * 2.0;

		if( mouseButtonStatus[ LEFT_MB ] )
		{
			sgct::Engine::getMousePos( &mouseXPos[0], &mouseYPos[0] );
			mouseDiff[ X ] = mouseXPos[0] - mouseXPos[1];
			mouseDiff[ Y ] = mouseYPos[0] - mouseYPos[1];
		}
		else
		{
			mouseDiff[ X ] = 0;
			mouseDiff[ Y ] = 0;
		}

		static float rotation [] = {0.0f, 0.0f, 0.0f};
		rotation[ Y ] -= (static_cast<float>(mouseDiff[ X ]) * rotationSpeed * static_cast<float>(dt));
		rotation[ X ] += (static_cast<float>(mouseDiff[ Y ]) * rotationSpeed * static_cast<float>(dt));

		glm::mat4 ViewRotate = glm::eulerAngleXY( rotation[ X ], rotation[ Y ] );

		view = glm::inverse(glm::mat3(ViewRotate)) * glm::vec3(0.0f, 0.0f, 1.0f);
		up = glm::inverse(glm::mat3(ViewRotate)) * glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 right = glm::cross(view, up);

		if( arrowButtonStatus[FORWARD] )
			position += (navigation_speed * static_cast<float>(dt) * view);
		if( arrowButtonStatus[BACKWARD] )
			position -= (navigation_speed * static_cast<float>(dt) * view);
		if( arrowButtonStatus[LEFT] )
			position -= (navigation_speed * static_cast<float>(dt) * right);
		if( arrowButtonStatus[RIGHT] )
			position += (navigation_speed * static_cast<float>(dt) * right);
		if( arrowButtonStatus[UPWARD] )
			position -= (navigation_speed * static_cast<float>(dt) * up);
		if( arrowButtonStatus[DOWNWARD] )
			position += (navigation_speed * static_cast<float>(dt) * up);

		/*
			To get a first person camera, the world needs
			to be transformed around the users head.

			This is done by:
			1, Transform the user to coordinate system origin
			2, Apply transformation
			3, Transform the user back to original position

			However, mathwise this process need to be reversed
			due to the matrix multiplication order.
		*/

		//3. transform user back to original position
		xform = glm::translate( glm::mat4(1.0f), sgct::Engine::getUserPtr()->getPos() );
		//2. apply transformation
		xform *= (ViewRotate * glm::translate( glm::mat4(1.0f), position ));
		//1. transform user to coordinate system origin
		xform *= glm::translate( glm::mat4(1.0f), -sgct::Engine::getUserPtr()->getPos() );

	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setWireframe(wireframe);
	gEngine->setDisplayInfoVisibility(info);
	gEngine->setStatsGraphVisibility(stats);

	if( culling )
		mModel->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
	else
		mModel->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

	if( takeScreenshot )
	{
		gEngine->takeScreenshot();
		takeScreenshot = false;
	}

	mSceneScale->setMatrix( osg::Matrix::scale( scale, scale, scale ) );

	light ? mRootNode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE) :
		mRootNode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

	mSceneTrans->setMatrix(osg::Matrix::rotate( glm::radians(animateAngle), 0.0, 1.0, 0.0));
	mSceneTrans->postMult(osg::Matrix( glm::value_ptr(xform) ));

	//transform to scene transformation from configuration file
	mSceneTrans->postMult( osg::Matrix( glm::value_ptr( gEngine->getSceneTransform() ) ));

	mViewer->advance(dt);

	//traverse if there are any tasks to do
	if (!mViewer->done())
	{
		mViewer->eventTraversal();
		//update travelsal needed for pagelod object like terrain data etc.
		mViewer->updateTraversal();
	}
}

void myDrawFun()
{
	const int * curr_vp = gEngine->getActiveViewport();
	mViewer->getCamera()->setViewport(curr_vp[0], curr_vp[1], curr_vp[2], curr_vp[3]);
	mViewer->getCamera()->setProjectionMatrix( osg::Matrix( glm::value_ptr(gEngine->getActiveProjectionMatrix() ) ));

	mViewer->renderingTraversals();
	
	if( gEngine->isMaster() )
	{
		sgct_text::print(sgct_text::FontManager::Instance()->GetDefaultFont(), 20, 35, "Scale: %.10f", scale);
		sgct_text::print(sgct_text::FontManager::Instance()->GetDefaultFont(), 20, 20, "Pos: %.3f %.3f %.3f", position.x, position.y, position.z);
	}
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( dt );
	sgct::SharedData::Instance()->writeDouble( animateAngle );
	for(int i=0; i<16; i++)
		sgct::SharedData::Instance()->writeFloat( glm::value_ptr(xform)[i] );
	sgct::SharedData::Instance()->writeBool( wireframe );
	sgct::SharedData::Instance()->writeBool( info );
	sgct::SharedData::Instance()->writeBool( stats );
	sgct::SharedData::Instance()->writeBool( takeScreenshot );
	sgct::SharedData::Instance()->writeBool( light );
	sgct::SharedData::Instance()->writeBool( culling );
	sgct::SharedData::Instance()->writeFloat( scale );
}

void myDecodeFun()
{
	dt = sgct::SharedData::Instance()->readDouble();
	animateAngle = sgct::SharedData::Instance()->readDouble();
	for(int i=0; i<16; i++)
		glm::value_ptr(xform)[i] = sgct::SharedData::Instance()->readFloat();
	wireframe = sgct::SharedData::Instance()->readBool();
	info = sgct::SharedData::Instance()->readBool();
	stats = sgct::SharedData::Instance()->readBool();
	takeScreenshot = sgct::SharedData::Instance()->readBool();
	light = sgct::SharedData::Instance()->readBool();
	culling = sgct::SharedData::Instance()->readBool();
	scale = sgct::SharedData::Instance()->readFloat();
}

void myCleanUpFun()
{
	sgct::MessageHandler::Instance()->print("Cleaning up osg data...\n");
	delete mViewer;
	mViewer = NULL;
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case 'C':
			if( modifierKey && action == SGCT_PRESS)
				culling = !culling;
			break;

		case 'S':
			if( modifierKey && action == SGCT_PRESS)
				stats = !stats;
			else
				arrowButtonStatus[BACKWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case 'I':
			if(action == SGCT_PRESS)
				info = !info;
			break;

		case 'L':
			if(action == SGCT_PRESS)
				light = !light;
			break;

		case 'W':
			if( modifierKey && action == SGCT_PRESS)
				wireframe = !wireframe;
			else
				arrowButtonStatus[FORWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case 'P':
		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot = true;
			break;

		case SGCT_KEY_UP:
			arrowButtonStatus[FORWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case SGCT_KEY_DOWN:
			arrowButtonStatus[BACKWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case SGCT_KEY_RIGHT:
		case 'D':
			arrowButtonStatus[RIGHT] = (action == SGCT_PRESS ? true : false);
			break;

		case SGCT_KEY_LEFT:
		case 'A':
			arrowButtonStatus[LEFT] = (action == SGCT_PRESS ? true : false);
			break;

		case SGCT_KEY_SPACE:
			if(action == SGCT_PRESS)
				animate = !animate;
			break;

		case SGCT_KEY_LCTRL:
		case SGCT_KEY_RCTRL:
			modifierKey = (action == SGCT_PRESS ? true : false);
			break;

		case 'Q':
			arrowButtonStatus[DOWNWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case 'E':
			arrowButtonStatus[UPWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case SGCT_KEY_PAGEUP:
			if(action == SGCT_PRESS)
				scale *= 2.0f;
			break;

		case SGCT_KEY_PAGEDOWN:
			if(action == SGCT_PRESS)
				scale /= 2.0f;
			break;
		}
	}
}

void mouseButtonCallback(int button, int action)
{
	if( gEngine->isMaster() )
	{
		switch( button )
		{
		case SGCT_MOUSE_BUTTON_LEFT:
			mouseButtonStatus[ LEFT_MB ] = (action == SGCT_PRESS ? true : false);
			
			//set refPos
			sgct::Engine::getMousePos( &mouseXPos[1], &mouseYPos[1] );
			break;
		}
	}
}

void initOSG()
{
	mRootNode = new osg::Group();
	osg::Referenced::setThreadSafeReferenceCounting(true);

	// Create the osgViewer instance
	mViewer = new osgViewer::Viewer;

	mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

	// Set up osgViewer::GraphicsWindowEmbedded for this context
	osg::ref_ptr< ::osg::GraphicsContext::Traits > traits =
      new osg::GraphicsContext::Traits;

	osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> graphicsWindow =
      new osgViewer::GraphicsWindowEmbedded(traits.get());

	mViewer->getCamera()->setGraphicsContext(graphicsWindow.get());
	mViewer->getCamera()->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);
	mViewer->getCamera()->setClearColor( osg::Vec4( 0.0f, 0.0f, 0.0f, 0.0f) );

	//disable osg from clearing the buffers that will be done by sgct
	GLbitfield tmpMask = mViewer->getCamera()->getClearMask();
	mViewer->getCamera()->setClearMask(tmpMask & (~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)));

	mViewer->setSceneData(mRootNode.get());
}

void setupLightSource()
{
	osg::Light * light0 = new osg::Light;
	osg::Light * light1 = new osg::Light;
	osg::LightSource* lightSource0 = new osg::LightSource;
	osg::LightSource* lightSource1 = new osg::LightSource;

	light0->setLightNum( 0 );
	light0->setPosition( osg::Vec4( 5.0f, 5.0f, 10.0f, 1.0f ) );
	light0->setAmbient( osg::Vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	light0->setDiffuse( osg::Vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	light0->setSpecular( osg::Vec4( 0.1f, 0.1f, 0.1f, 1.0f ) );
	light0->setConstantAttenuation( 1.0f );

	lightSource0->setLight( light0 );
    lightSource0->setLocalStateSetModes( osg::StateAttribute::ON );
	lightSource0->setStateSetModes( *(mRootNode->getOrCreateStateSet()), osg::StateAttribute::ON );

	light1->setLightNum( 1 );
	light1->setPosition( osg::Vec4( -5.0f, -2.0f, 10.0f, 1.0f ) );
	light1->setAmbient( osg::Vec4( 0.2f, 0.2f, 0.2f, 1.0f ) );
	light1->setDiffuse( osg::Vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	light1->setSpecular( osg::Vec4( 0.2f, 0.2f, 0.2f, 1.0f ) );
	light1->setConstantAttenuation( 1.0f );

	lightSource1->setLight( light1 );
    lightSource1->setLocalStateSetModes( osg::StateAttribute::ON );
	lightSource1->setStateSetModes( *(mRootNode->getOrCreateStateSet()), osg::StateAttribute::ON );

	mRootNode->addChild( lightSource0 );
}
