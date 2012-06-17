#include "sgct.h"
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/Matrix>
#include <osg/Transform>
#include <osg/MatrixTransform>

sgct::Engine * gEngine;

//Not using ref pointers enables
//more controlled termination
//and prevents segfault on Linux
osgViewer::Viewer * mViewer;
osg::ref_ptr<osg::Group> mRootNode;
osg::ref_ptr<osg::MatrixTransform> mSceneTrans;

//callbacks
void myInitOGLFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myDrawFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
void keyCallback(int key, int action);

//other functions
void initOSG();

//variables to share across cluster
double curr_time = 0.0;
double dist = -10.0;
bool wireframe = false;
bool info = false;
bool stats = false;
bool takeScreenshot = false;

//other var
bool arrowButtons[4];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT };
const double navigation_speed = 5.0;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );

	for(int i=0; i<4; i++)
		arrowButtons[i] = false;

	if( !gEngine->init() )
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
	osg::ref_ptr<osg::Node>            mModel;

	mSceneTrans = new osg::MatrixTransform();
	mModelTrans  = new osg::MatrixTransform();

	//rescale normals during a scale transformation
	mModelTrans->getOrCreateStateSet()->setMode( GL_RESCALE_NORMAL, osg::StateAttribute::ON );

	//rotate osg coordinate system to match sgct
	mModelTrans->preMult(osg::Matrix::rotate(glm::radians(-90.0f),
                                            1.0f, 0.0f, 0.0f));
	//foot to meter conversion
	mModelTrans->postMult(osg::Matrix::scale( 0.3048f, 0.3048f, 0.3048f));

	mRootNode->addChild( mSceneTrans.get() );
	mSceneTrans->addChild( mModelTrans.get() );

	sgct::MessageHandler::Instance()->print("Loading model 'cessnafire.osg'...\n");
	mModel = osgDB::readNodeFile("cessnafire.osg");

	if ( mModel.valid() )
	{
		sgct::MessageHandler::Instance()->print("Model loaded successfully!\n");
		mModelTrans->addChild(mModel.get());

		//get the bounding sphere
		osg::BoundingSphere bs;
		bs = mModel->getBound();
		osg::Vec3f tmpVec;
		tmpVec = bs.center();

		sgct::MessageHandler::Instance()->print("Model bounding sphere center:\tx=%f\ty=%f\tz=%f\n", tmpVec[0], tmpVec[1], tmpVec[2] );
		sgct::MessageHandler::Instance()->print("Model bounding sphere radius:\t%f\n", bs.radius() );
	}
	else
		sgct::MessageHandler::Instance()->print("Failed to read model!\n");
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		curr_time = sgct::Engine::getTime();

		if( arrowButtons[FORWARD] )
			dist += (navigation_speed * gEngine->getDt());
		if( arrowButtons[BACKWARD] )
			dist -= (navigation_speed * gEngine->getDt());

	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setWireframe(wireframe);
	gEngine->setDisplayInfoVisibility(info);
	gEngine->setStatsGraphVisibility(stats);

	if( takeScreenshot )
	{
		gEngine->takeScreenshot();
		takeScreenshot = false;
	}

	mSceneTrans->setMatrix(osg::Matrix::rotate( glm::radians(curr_time * 8.0), 0.0, 1.0, 0.0));
	mSceneTrans->postMult(osg::Matrix::translate(0.0, 0.0, dist));

	mViewer->advance(curr_time);

	//traverse if there are any tasks to do
	if (!mViewer->done())
      mViewer->eventTraversal();
}

void myDrawFun()
{
	const int * curr_vp = gEngine->getActiveViewport();
	mViewer->getCamera()->setViewport(curr_vp[0], curr_vp[1], curr_vp[2], curr_vp[3]);
	mViewer->getCamera()->setProjectionMatrix( osg::Matrix( glm::value_ptr(gEngine->getActiveProjectionMatrix() ) ));

	mViewer->renderingTraversals();
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble( curr_time );
	sgct::SharedData::Instance()->writeDouble( dist );
	sgct::SharedData::Instance()->writeBool( wireframe );
	sgct::SharedData::Instance()->writeBool( info );
	sgct::SharedData::Instance()->writeBool( stats );
	sgct::SharedData::Instance()->writeBool( takeScreenshot );
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
	dist = sgct::SharedData::Instance()->readDouble();
	wireframe = sgct::SharedData::Instance()->readBool();
	info = sgct::SharedData::Instance()->readBool();
	stats = sgct::SharedData::Instance()->readBool();
	takeScreenshot = sgct::SharedData::Instance()->readBool();
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
		case 'S':
			if(action == GLFW_PRESS)
				stats = !stats;
			break;

		case 'I':
			if(action == GLFW_PRESS)
				info = !info;
			break;

		case 'W':
			if(action == GLFW_PRESS)
				wireframe = !wireframe;
			break;

		case 'Q':
			if(action == GLFW_PRESS)
				gEngine->terminate();
			break;

		case 'P':
		case GLFW_KEY_F10:
			if(action == GLFW_PRESS)
				takeScreenshot = true;
			break;

		case GLFW_KEY_UP:
			arrowButtons[FORWARD] = (action == GLFW_PRESS ? true : false);
			break;

		case GLFW_KEY_DOWN:
			arrowButtons[BACKWARD] = (action == GLFW_PRESS ? true : false);
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

	//mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

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
