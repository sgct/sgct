#include "sgct.h"

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/MatrixTransform>
#include <osg/ComputeBoundsVisitor>
//#include <osg/PositionAttitudeTransform>
//#include "AnimationCallbacks.h"
#include "Animation.h"
#include <osgUtil/Optimizer>
#include <glm/gtx/euler_angles.hpp>

sgct::Engine * gEngine;

//Not using ref pointers enables
//more controlled termination
//and prevents segfault on Linux
osgViewer::Viewer * mViewer = NULL;
osg::ref_ptr<osg::Group>			mRootNode;
osg::ref_ptr<osg::FrameStamp> mFrameStamp; //to sync osg animations across cluster
osg::ref_ptr<osg::MatrixTransform>	mSceneTrans;
osg::ref_ptr<osg::MatrixTransform>	mSceneScale;
std::vector< osg::ref_ptr<osg::Node> > mModels;
std::vector< std::string > mFilenames;
osg::ref_ptr<osg::Switch> mSwitch;
//osg::ref_ptr<osg::AnimationPath> animationPath;
//osg::PositionAttitudeTransform* animation;
/*osg::ref_ptr<osg::MatrixTransform> mAnimatedTransform;
osg::ref_ptr<osgAnimation::Vec3CubicBezierKeyframeContainer> mKeyFrames;
AnimtkUpdateCallback* mAnimationCallback = NULL;*/

AnimationData ad;

Animation mAnimation(true);

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
sgct::SharedDouble dt(0.0);
sgct::SharedDouble animateAngle(0.0);
sgct::SharedObject<glm::mat4> xform(glm::mat4(1.0f));
sgct::SharedFloat scale(1.0f);
sgct::SharedBool wireframe(false);
sgct::SharedBool info(false);
sgct::SharedBool stats(false);
sgct::SharedBool takeScreenshot(false);
sgct::SharedBool light(true);
sgct::SharedBool culling(true);

//local only
bool recordMode = false;
bool addAnimationSample = false;
bool startRecoding = false;
double startRecodingTime = 0.0;

//other var
double current_time = 0.0;
bool animate = false;
glm::vec3 view(0.0f, 0.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
//glm::vec3 position(0.00f, 1.60f, -1.170f);
//glm::vec3 position( -0.741910f, -0.303285f, 5.952151f );
glm::vec3 position( 0.0f, 0.0f, 0.0f );
bool arrowButtonStatus[6];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT, UPWARD, DOWNWARD };
bool mouseButtonStatus[3];
bool modifierKey = false;
enum mouseButtons { LEFT_MB = 0, MIDDLE_MB, RIGHT_MB };
enum axes { X = 0, Y, Z};
float rotationSpeed = 0.003f;
float navigation_speed = 0.5f;
double mouseDiff[] = { 0.0, 0.0 };
/* Stores the positions that will be compared to measure the difference. */
double mouseXPos[] = { 0.0, 0.0 };
double mouseYPos[] = { 0.0, 0.0 };

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
	//gEngine->setFisheyeClearColor(0.0f, 0.0f, 0.0f);

	//fix incompability with warping and OSG
	sgct_core::ClusterManager::instance()->setMeshImplementation( sgct_core::ClusterManager::DISPLAY_LIST );

	for(int i=0; i<6; i++)
		arrowButtonStatus[i] = false;

	for(int i=0; i<3; i++)
		mouseButtonStatus[i] = false;

	if( !gEngine->init() )
	//if( !gEngine->init(sgct::Engine::OSG_Encapsulation_Mode) )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	//gEngine->setScreenCaptureFormat( sgct_core::ScreenCapture::TGA );

	sgct::SharedData::instance()->setEncodeFunction( myEncodeFun );
	sgct::SharedData::instance()->setDecodeFunction( myDecodeFun );

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
	mSwitch = new osg::Switch();

	//rotate osg coordinate system to match sgct
	float tmpf = scale.getVal();
	mSceneScale->setMatrix( osg::Matrix::scale(tmpf, tmpf, tmpf) );
	mModelTrans->setMatrix(osg::Matrix::rotate(glm::radians(-90.0f),
                                            1.0f, 0.0f, 0.0f));

	mRootNode->getOrCreateStateSet()->setMode( GL_RESCALE_NORMAL, osg::StateAttribute::ON );

	//animationPath = new osg::AnimationPath;
	//animationPath->setLoopMode(osg::AnimationPath::NO_LOOPING);
	//animationPath->setLoopMode(osg::AnimationPath::LOOP);

	//animation = new osg::PositionAttitudeTransform;
	//animation->addChild( mSceneScale.get() );

	/*mAnimatedTransform = new osg::MatrixTransform;
	mAnimatedTransform->addChild( mSceneScale.get() );
	mAnimationCallback = new AnimtkUpdateCallback;
	mAnimatedTransform->setUpdateCallback( mAnimationCallback );
	mKeyFrames = mAnimationCallback->_sampler->getOrCreateKeyframeContainer();*/

	//animation stuff
	mAnimation.getOrCreateAnimation()->addChild( mSceneScale.get() );
	mRootNode->setUpdateCallback( mAnimation.getManager() );
	ad.mSpeed = 0.5;

	mRootNode->addChild( mSwitch.get() );
	mSwitch->insertChild( 0, mSceneTrans.get() );
	mSwitch->insertChild( 1, mAnimation.getOrCreateAnimation() );
	mSceneTrans->addChild( mSceneScale.get() );
	mSceneScale->addChild( mModelTrans.get() );

	mSwitch->setSingleChildOn( 0 );

	//mFilenames.push_back( std::string("iss_all_maps_no_opacity.ive") );
	mFilenames.push_back( std::string("Y:\\models\\iss\\ESA-ESTEC_ISS-3DDB\\ESA-ESTEC_ISS_3DDB-20121030\\20121030\\Int\\FLT\\models_current\\iss_esa_int_stage5.ive"));
	//mFilenames.push_back( std::string("Y:\\models\\test_harmony_20130219\\harmony_test\\harmony_test.ive"));
	//std::string filename = "Y:\\models\\iss\\ESA-ESTEC_ISS-3DDB\\ESA-ESTEC_ISS_3DDB-20121030\\20121030\\Ext\\FLT\\iss_esa_ext_stage5.ive"

	//osgDB::ReaderWriter::Options * options = new osgDB::ReaderWriter::Options("dds_flip");

	for(size_t i=0; i<mFilenames.size(); i++)
	{
		osg::Node * tmpNode = NULL;
		tmpNode = new (std::nothrow) osg::Node();
		sgct::MessageHandler::instance()->print("Loading model '%s'...\n", mFilenames[i].c_str());
		tmpNode = osgDB::readNodeFile( mFilenames[i] );

		if ( tmpNode != NULL )
		{
			sgct::MessageHandler::instance()->print("Model loaded successfully!\n");
			mModelTrans->addChild( tmpNode );

			//get the bounding box
			osg::ComputeBoundsVisitor cbv;
			osg::BoundingBox &bb(cbv.getBoundingBox());
			tmpNode->accept( cbv );

			osg::Vec3f tmpVec;
			tmpVec = bb.center();

			//translate model center to origin
			//mModelTrans->postMult(osg::Matrix::translate( -tmpVec[0], -tmpVec[1], -tmpVec[2] ) );

			sgct::MessageHandler::instance()->print("Model bounding sphere center:\tx=%f\ty=%f\tz=%f\n", tmpVec[0], tmpVec[1], tmpVec[2] );
			sgct::MessageHandler::instance()->print("Model bounding sphere radius:\t%f\n", bb.radius() );
			sgct::MessageHandler::instance()->print("Model bounding xMin:%.3f\txMax:%.3f\txSize: %.3f\n", bb.xMin(), bb.xMax(), bb.xMax() - bb.xMin() );
			sgct::MessageHandler::instance()->print("Model bounding yMin:%.3f\tyMax:%.3f\tySize: %.3f\n", bb.yMin(), bb.yMax(), bb.yMax() - bb.yMin() );
			sgct::MessageHandler::instance()->print("Model bounding zMin:%.3f\tzMax:%.3f\tzSize: %.3f\n", bb.zMin(), bb.zMax(), bb.zMax() - bb.zMin() );

			mModels.push_back( tmpNode );
		}
		else
		{
			sgct::MessageHandler::instance()->print("Failed to read model!\n");

		}
	}

	setupLightSource();

	//optimize scenegraph
	osgUtil::Optimizer optimizer;
	optimizer.optimize(mRootNode.get());
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		//dt.setVal(gEngine->getDt());
		dt.setVal(1.0 / 30.0); //30 fps
		double deltaTime = dt.getVal();

		if( animate )
			animateAngle.setVal( animateAngle.getVal() + deltaTime * 2.0);

		if( mouseButtonStatus[ LEFT_MB ] )
		{
			sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[0], &mouseYPos[0] );
			mouseDiff[ X ] = mouseXPos[0] - mouseXPos[1];
			mouseDiff[ Y ] = mouseYPos[0] - mouseYPos[1];
		}
		else
		{
			mouseDiff[ X ] = 0;
			mouseDiff[ Y ] = 0;
		}

		static float rotation [] = {0.0f, 0.0f, 0.0f};
		rotation[ Y ] -= (static_cast<float>(mouseDiff[ X ]) * rotationSpeed * static_cast<float>(deltaTime));
		rotation[ X ] += (static_cast<float>(mouseDiff[ Y ]) * rotationSpeed * static_cast<float>(deltaTime));

		glm::mat4 ViewRotate = glm::eulerAngleXY( rotation[ X ], rotation[ Y ] );

		view = glm::inverse(glm::mat3(ViewRotate)) * glm::vec3(0.0f, 0.0f, 1.0f);
		up = glm::inverse(glm::mat3(ViewRotate)) * glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 right = glm::cross(view, up);

		if( arrowButtonStatus[FORWARD] )
			position += (navigation_speed * static_cast<float>(deltaTime) * view);
		if( arrowButtonStatus[BACKWARD] )
			position -= (navigation_speed * static_cast<float>(deltaTime) * view);
		if( arrowButtonStatus[LEFT] )
			position -= (navigation_speed * static_cast<float>(deltaTime) * right);
		if( arrowButtonStatus[RIGHT] )
			position += (navigation_speed * static_cast<float>(deltaTime) * right);
		if( arrowButtonStatus[UPWARD] )
			position -= (navigation_speed * static_cast<float>(deltaTime) * up);
		if( arrowButtonStatus[DOWNWARD] )
			position += (navigation_speed * static_cast<float>(deltaTime) * up);

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
		glm::mat4 result;
		result = glm::translate( glm::mat4(1.0f), sgct::Engine::getDefaultUserPtr()->getPos() );
		//2. apply transformation
		result *= (ViewRotate * glm::translate( glm::mat4(1.0f), position ));
		//1. transform user to coordinate system origin
		result *= glm::translate( glm::mat4(1.0f), -sgct::Engine::getDefaultUserPtr()->getPos() );

		xform.setVal( result );
	}
}

void myPostSyncPreDrawFun()
{
	gEngine->setWireframe(wireframe.getVal());
	gEngine->setDisplayInfoVisibility(info.getVal());
	gEngine->setStatsGraphVisibility(stats.getVal());

	current_time += dt.getVal();

	//set correct subtree
	//recordMode ? mSwitch->setSingleChildOn(1) : mSwitch->setSingleChildOn(0);
	static double t0 = 0.0;
	if(recordMode)
	{
		if( startRecoding )
		{
			//animation->setUpdateCallback(new osg::AnimationPathCallback(animationPath, 0.0, 1.0));
			startRecoding = false;
			startRecodingTime = current_time;
			t0 = sgct::Engine::getTime();

			if( ad.mPositions.size() > 1 )
			{
				double time = 0.0;
				double dist;
				const double timeStep = 2.0;
				const float weight = 1.0f/3.0f;

				if( mAnimation.isCubic() )
				{
					//first
					mAnimation.addCubicPositionKeyFrame(time,
						ad.mPositions[0],
                        ad.mPositions[0],
                        ad.mPositions[0] + (ad.mPositions[1] - ad.mPositions[0]) * weight);

					mAnimation.addQuatKeyFrame(time, ad.mQuats[0]);

					for(size_t i=1; i<ad.mPositions.size()-1; i++)
					{
						//dist = static_cast<double>((ad.mPositions[i] - ad.mPositions[i-1]).length());
						//time += dist/ad.speed; //to get constant speed

						time += timeStep;

						mAnimation.addCubicPositionKeyFrame(time,
							ad.mPositions[i] - (ad.mPositions[i] - ad.mPositions[i-1]) * weight,
							ad.mPositions[i],
                            ad.mPositions[i] + (ad.mPositions[i+1] - ad.mPositions[i]) * weight);
						mAnimation.addQuatKeyFrame(time, ad.mQuats[i]);
					}

					//dist = static_cast<double>((ad.mPositions.back() - ad.mPositions[ ad.mPositions.size()-2 ]).length());
					//time += dist/ad.speed;
					time += timeStep;

					//last
					mAnimation.addCubicPositionKeyFrame(time,
						ad.mPositions.back() - (ad.mPositions.back() - ad.mPositions[ ad.mPositions.size()-2 ]) * weight,
                        ad.mPositions.back(),
                        ad.mPositions.back());

					mAnimation.addQuatKeyFrame(time, ad.mQuats.back());
				}
				else
				{
					mAnimation.addPositionKeyFrame(time, ad.mPositions[0]);
					mAnimation.addQuatKeyFrame(time, ad.mQuats[0]);

					for(size_t i=1; i<ad.mPositions.size(); i++)
					{
						dist = static_cast<double>((ad.mPositions[i] - ad.mPositions[i-1]).length());
						time += dist/ad.mSpeed; //to get constant speed
						mAnimation.addPositionKeyFrame(time, ad.mPositions[i]);
						mAnimation.addQuatKeyFrame(time, ad.mQuats[i]);
					}
				}

				mAnimation.play();
			}
		}

		mSwitch->setSingleChildOn(1);

		if( !mAnimation.isPlaying() ) //animation has ended
		{
			recordMode = false;
			//mingw cannot handle %lf so lets convert to float %f
			sgct::MessageHandler::instance()->print("Info: Rendering took %fs\n", static_cast<float>(sgct::Engine::getTime() - t0));
		}
		else
		{
			mSwitch->setSingleChildOn(1);
			//gEngine->takeScreenshot();
		}
	}
	else
	{
		mSwitch->setSingleChildOn(0);
	}

	for(size_t i = 0; i<mModels.size(); i++)
	{
		culling.getVal() ?
			mModels[i]->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE) :
			mModels[i]->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
	}

	if( takeScreenshot.getVal() )
	{
		gEngine->takeScreenshot();
		takeScreenshot.setVal(false);
	}

	mSceneScale->setMatrix( osg::Matrix::scale( scale.getVal(), scale.getVal(), scale.getVal() ) );

	light.getVal() ? mRootNode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE) :
		mRootNode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

	mSceneTrans->setMatrix(osg::Matrix::rotate( glm::radians(animateAngle.getVal()), 0.0, 1.0, 0.0));
	
    mSceneTrans->postMult(osg::Matrix( glm::value_ptr(xform.getVal()) ));

	//transform to scene transformation from configuration file
	mSceneTrans->postMult( osg::Matrix( glm::value_ptr( gEngine->getModelMatrix() ) ));

	if( addAnimationSample )
	{
		addAnimationSample = false;
		//static double timeStamp = 0.0;
		//animationPath->insert(timeStamp, osg::AnimationPath::ControlPoint( mSceneTrans->getMatrix().getTrans(),
		//	mSceneTrans->getMatrix().getRotate()) );
		//sgct::MessageHandler::instance()->print("Adding animation sample at: %lf (total duration: %lfs)\n", timeStamp, animationPath->getPeriod());

		ad.mPositions.push_back( osg::Vec3( position[0], position[1], position[2] ) );
		ad.mQuats.push_back( mSceneTrans->getMatrix().getRotate() );
		sgct::MessageHandler::instance()->print("Adding animation sample.\n");
		//timeStamp += 1.0;
	}

	//double tmpTime = gEngine->getTime();

	//update the frame stamp in the viewer to sync all
	//time based events in osg
	mFrameStamp->setFrameNumber( gEngine->getCurrentFrameNumber() );
	mFrameStamp->setReferenceTime( current_time );
	mFrameStamp->setSimulationTime( current_time );
	mViewer->setFrameStamp( mFrameStamp );
	mViewer->advance( current_time ); //update

	//traverse if there are any tasks to do
	if (!mViewer->done())
	{
		mViewer->eventTraversal();
		//update travelsal needed for pagelod object like terrain data etc.
		mViewer->updateTraversal();
	}

	//fprintf(stderr, "Post Sync OSG time: %.0f ms\n", (gEngine->getTime() - tmpTime) * 1000.0);
}

void myDrawFun()
{
	const int * curr_vp = gEngine->getActiveViewportPixelCoords();
	mViewer->getCamera()->setViewport(curr_vp[0], curr_vp[1], curr_vp[2], curr_vp[3]);
	mViewer->getCamera()->setProjectionMatrix( osg::Matrix( glm::value_ptr(gEngine->getActiveViewProjectionMatrix() ) ));

	//double tmpTime = gEngine->getTime();
	mViewer->renderingTraversals();
	//fprintf(stderr, "Draw OSG time: %.0f ms\n", (gEngine->getTime() - tmpTime) * 1000.0);

	/*if( gEngine->isMaster() )
	{
		sgct_text::print(sgct_text::FontManager::instance()->GetDefaultFont(), 20, 35, "Scale: %.10f", scale);
		sgct_text::print(sgct_text::FontManager::instance()->GetDefaultFont(), 20, 20, "Pos: %.3f %.3f %.3f", position.x, position.y, position.z);
	}*/
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble( &dt );
	sgct::SharedData::instance()->writeDouble( &animateAngle );
	sgct::SharedData::instance()->writeObj( &xform );
	sgct::SharedData::instance()->writeBool( &wireframe );
	sgct::SharedData::instance()->writeBool( &info );
	sgct::SharedData::instance()->writeBool( &stats );
	sgct::SharedData::instance()->writeBool( &takeScreenshot );
	sgct::SharedData::instance()->writeBool( &light );
	sgct::SharedData::instance()->writeBool( &culling );
	sgct::SharedData::instance()->writeFloat( &scale );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble( &dt );
	sgct::SharedData::instance()->readDouble( &animateAngle );
	sgct::SharedData::instance()->readObj( &xform );
	sgct::SharedData::instance()->readBool( &wireframe );
	sgct::SharedData::instance()->readBool( &info );
	sgct::SharedData::instance()->readBool( &stats );
	sgct::SharedData::instance()->readBool( &takeScreenshot );
	sgct::SharedData::instance()->readBool( &light );
	sgct::SharedData::instance()->readBool( &culling );
	sgct::SharedData::instance()->readFloat( &scale );
}

void myCleanUpFun()
{
	sgct::MessageHandler::instance()->print("Cleaning up osg data...\n");
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
				culling.toggle();
			break;

		case 'S':
			if( modifierKey && action == SGCT_PRESS)
				ad.save("Test.path");
			else
				arrowButtonStatus[BACKWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case 'I':
			if(action == SGCT_PRESS)
				info.toggle();
			break;

		case 'L':
			if(action == SGCT_PRESS)
				light.toggle();
			break;

		case 'W':
			if( modifierKey && action == SGCT_PRESS)
				wireframe.toggle();
			else
				arrowButtonStatus[FORWARD] = (action == SGCT_PRESS ? true : false);
			break;

		case 'P':
		case SGCT_KEY_F10:
			if(action == SGCT_PRESS)
				takeScreenshot.setVal( true );
			break;

		case 'R':
			if(action == SGCT_PRESS)
			{
				recordMode = !recordMode;
				if(recordMode)
					startRecoding = true;
			}
			break;

		case 'O':
			if( modifierKey && action == SGCT_PRESS)
				ad.read("Test.path");
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

		case 'B':
			sgct::MessageHandler::instance()->print("Pos: %f %f %f\n", position.x, position.y, position.z);
			break;

		case SGCT_KEY_SPACE:
			if(action == SGCT_PRESS)
			{
				if(modifierKey)
					animate = !animate;
				else
					addAnimationSample = true;
			}
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
			{
				scale.setVal( scale.getVal() * 2.0f );
				sgct::MessageHandler::instance()->print("Scale set to %f\n", scale.getVal());
			}
			break;

		case SGCT_KEY_PAGEDOWN:
			if(action == SGCT_PRESS)
			{
				scale.setVal( scale.getVal() / 2.0f );
				sgct::MessageHandler::instance()->print("Scale set to %f\n", scale.getVal());
			}
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
			sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[1], &mouseYPos[1] );
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

	// Create a time stamp instance
	mFrameStamp	= new osg::FrameStamp();

	mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
	//mViewer->setEndBarrierPosition( osgViewer::ViewerBase::BeforeSwapBuffers );
	//mViewer->setEndBarrierPosition( osgViewer::ViewerBase::AfterSwapBuffers );

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
