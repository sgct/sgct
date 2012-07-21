#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include "sgct.h"
//#include "sgct/PLYReader.h"
#define EXTENDED_SIZE 40000

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myPostDrawFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

void keyCallback(int key, int action);
void externalControlCallback(const char * receivedChars, int size, int clientId);

//variables to share across cluster
double dt = 0.0;
double curr_time = 0.0;
bool showFPS = false;
bool extraPackages = false;
bool barrier = false;
bool resetCounter = false;
bool stats = false;
bool takeScreenshot = false;
bool slowRendering = false;
float extraData[EXTENDED_SIZE];
unsigned char flags = 0;
std::string mySharedString;

void drawGrid(float size, int steps);

class TestC
{
public:
	TestC()
	{
		d1 = 1.0;
		d2 = 1.0;
		f1 = 5.0f;
		f2 = 5.0f;
		i1 = -1;
		i2 = -1;
	}

	double d1;
	double d2;
	float f1;
	float f2;
	int i1;
	int i2;
};

TestC myTestClass;

int main( int argc, char* argv[] )
{
	mySharedString = "Test!";

	gEngine = new sgct::Engine( argc, argv );
	gEngine->setClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setExternalControlCallback( externalControlCallback );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	
	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	//gEngine->setExternalControlBufferSize( 16384 );

	//double t0 = sgct::Engine::getTime();
	/*core_sgct::PLYReader myPLYReader;
	if( myPLYReader.readfile("bunny.ply") )
	{
		double duration = sgct::Engine::getTime() - t0;
		sgct::MessageHandler::Instance()->print("PLY File read in %f s\n", duration);
	}
	else
		sgct::MessageHandler::Instance()->print("Failed to parse ply file.\n");
		*/

	//allocate extra data
	if( gEngine->isMaster() )
		for(int i=0;i<EXTENDED_SIZE;i++)
			extraData[i] = static_cast<float>(rand()%500)/500.0f;

	sgct::SharedData::Instance()->setCompression(true);
	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
	gEngine->setPostDrawFunction( myPostDrawFun );

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	if(slowRendering)
		glfwSleep(1.0/5.0);

	glPushMatrix();

	/*if( core_sgct::Frustum::Mono == gEngine->getActiveFrustum() )
		sgct::MessageHandler::Instance()->print("Mono!\n");
	else if( core_sgct::Frustum::StereoLeftEye == gEngine->getActiveFrustum() )
		sgct::MessageHandler::Instance()->print("Left eye!\n");
	else
		sgct::MessageHandler::Instance()->print("Right eye!\n");*/

	//sgct::MessageHandler::Instance()->print("Mouse wheel: %d\n", sgct::Engine::getMouseWheel());
	//sgct::MessageHandler::Instance()->print("Right mouse button: %d\n", sgct::Engine::getMouseButton(GLFW_MOUSE_BUTTON_RIGHT));
	//sgct::MessageHandler::Instance()->print("Y key: %d\n", sgct::Engine::getKey('Y'));
	//sgct::Engine::setMousePos( rand()%600, rand()%400);

	glRotatef(static_cast<float>(curr_time)*10.0f, 0.0f, 1.0f, 0.0f);
	glScalef(1.0f, 0.5f, 1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glLineWidth(2.0);

	//draw a cube
	//bottom
	glBegin(GL_LINE_STRIP);
	glVertex3f( -1.0f, -1.0f, -1.0f);
	glVertex3f( 1.0f, -1.0f, -1.0f);
	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f( -1.0f, -1.0f, 1.0f);
	glVertex3f( -1.0f, -1.0f, -1.0f);
	glEnd();

	//top
	glBegin(GL_LINE_STRIP);
	glVertex3f( -1.0f, 1.0f, -1.0f);
	glVertex3f( 1.0f, 1.0f, -1.0f);
	glVertex3f( 1.0f, 1.0f, 1.0f);
	glVertex3f( -1.0f, 1.0f, 1.0f);
	glVertex3f( -1.0f, 1.0f, -1.0f);
	glEnd();

	//sides
	glBegin(GL_LINES);
	glVertex3f( -1.0f, -1.0f, -1.0f);
	glVertex3f( -1.0f, 1.0f, -1.0f);

	glVertex3f( 1.0f, -1.0f, -1.0f);
	glVertex3f( 1.0f, 1.0f, -1.0f);

	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f( 1.0f, 1.0f, 1.0f);

	glVertex3f( -1.0f, -1.0f, 1.0f);
	glVertex3f( -1.0f, 1.0f, 1.0f);
	glEnd();

	glPopMatrix();

	if( gEngine->getActiveFrustum() == core_sgct::Frustum::StereoLeftEye )
		Freetype::print(sgct::FontManager::Instance()->GetFont( "Verdana", 24 ), 100, 50, "Left");
	else if( gEngine->getActiveFrustum() == core_sgct::Frustum::StereoRightEye )
		Freetype::print(sgct::FontManager::Instance()->GetFont( "Verdana", 24 ), 100, 100, "Right");
	else if( gEngine->getActiveFrustum() == core_sgct::Frustum::Mono )
		Freetype::print(sgct::FontManager::Instance()->GetFont( "Verdana", 24 ), 100, 150, "Mono");

	/*Freetype::print(sgct::FontManager::Instance()->GetFont( "Verdana", 10 ), 20, 20, "Template test: %.3f %.3f %.3f %.3f %d %d",
		myTestClass.d1,
		myTestClass.d2,
		myTestClass.f1,
		myTestClass.f2,
		myTestClass.i1,
		myTestClass.i2);*/

	Freetype::print(sgct::FontManager::Instance()->GetFont( "Verdana", 10 ), 20, 20, "Size test: %f",
		extraData[EXTENDED_SIZE-1]);
	Freetype::print(sgct::FontManager::Instance()->GetFont( "Verdana", 10 ), 20, 0, "String: %s...",
		mySharedString.substr(0, 16).c_str());
	//drawGrid(10.0, 100);
}

void myPreSyncFun()
{	
	if( gEngine->isMaster() )
	{
		dt = gEngine->getDt();
		curr_time = gEngine->getTime();

		myTestClass.d1 = 95.0;
		myTestClass.d2 = 45.0;
		myTestClass.f1 = 34.0f;
		myTestClass.f2 = 3456.425f;
		myTestClass.i1 = 544;
		myTestClass.i2 = -345;

		mySharedString = "Long text... With many words..\
			Test tests tets dfgdnf jndjgdn fjgi fdgujdn f\
			fgdjfh gdfiuohgdfuigh dfuihg duighdui fhduifh gduif\
			fgdjkfhgdjkfh gdjfkhgdfhgeriu gnweuirgbe riugberu beu\
			geirohgerwasfjisogjioseg ioeriog eirog egj wiowj io\
			ldfgjeirohtw kjweo iohgrgohsofjwe io\
			sdiojgs iojoi hwiog wioweioj twepow ij\
			skjhfsdjkhf uihwuie twuisnbaf \
			sfhgwe gweyfe guyfgweygt jas jkfsdfgwei qwe we\
			rwere wew rter twrqew erytwe twtwet wetw wertwe\
			wet ewtwe tewt ehyert wgsdhyrwe twetsegsgywet\
			retre treher gsgdfhgwertegsxgfdgretgw  fdgdfe\
			rtre tery eryery ter teery erreger er\
			ter terte rre tesd gsdgert wegsdg ew twe\
			dsg wwet wtwe tegwet wegerwtwe twe wettwe\
			ghf  tyf   rf gvgfjc rffgc gffj tytyftr\
			jhff yr fddrdrtc ghfgjf tffyftyf hffgfgf\
			juhiugi yufuyf mgyugy ygffjhgjg jfgft hgftyf\
			ljjhgftxxr jhgyfvcgh gvhgfhg gfhgft ygffyycfcf\
			bvdbfd nfhgfj jfjfyj fhfhdr hfhdt fhgfht fhgfhtf\
			hgdt ddghf hgttjts fjdgndr dgrdssndt jtydsesfegr\
			yruruyr vuturf hffdhd sdgdjg jfjft dhdthd ghfhfh\
			jgygkjg jfjyfhf fhfhfdh dhdtd hftygdhd hfyfyufuy\
			fguuyfuyf ufkfkuyf hffjfvjy dtdyudc jhgyff dtd\
			hjfh fdsdrtsssd kgkfctd ngtyurfde rsdjglityr\
			dhfhgfvfj fhjfdhd drgdfh kgkgf jy fyfyyf hddh\
			hfjfj jfjrfyudfj jfjfjf jfyfjkfde dsrrtg ljhfset\
			qers3fddgyy77 fhfdgsd5 dhd55 hfcddssarr4e hfy7t\
			t464dydstrw jgfgkjgd cvbcg ghdtt ngjyd hfhfj yyu\
			gdyty jfhfh hft u jhfjfj jyfyjf yu yy iu\
			ddhyf jgfyjdj yurfyd fjfjf hdtydj jddyrf hgf\
			gdfd hdhd jyjh srsty syy jwje u7t5e hr7ej iutw\
			jrfj j 6ddhd sgfdh jjgjf trsyfd uytgfj dtrsd5r\
			rhfyu hfudht uhfst5 kgfy ggfd nvgtft ddrttcccy\
			lhgyjiuhtyh jgy6 hfhf hgdgdr hfsawar\
			sfwgr hrefsdg sfe seth stht stet stet ste\
			seg dht jtyt drt hhfjgk dhr dhjk dhtk hdht\
			fghgf utrg dgeh dghjr dhjt drth dghjr dhnuu\
			hft jgki crtdu lyug fchhy jhy ,jj gjfytf ggt";
	}
	/*else //slave only
	{
		for(int i=0; i<100; i++)
			sgct::MessageHandler::Instance()->print("Testing... %d!\n", i);
	}*/
}

void myPostSyncPreDrawFun()
{
	gEngine->setDisplayInfoVisibility( showFPS );
	gEngine->getWindowPtr()->setBarrier( barrier );
	gEngine->setStatsGraphVisibility( stats );

	if( takeScreenshot )
	{
		gEngine->takeScreenshot();
		takeScreenshot = false;
	}

	if(resetCounter)
	{
		gEngine->getWindowPtr()->resetSwapGroupFrameNumber();
	}
}

void myPostDrawFun()
{
	if( gEngine->isMaster() )
	{
		resetCounter = false;
	}
}

void myInitOGLFun()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
}

void myEncodeFun()
{
	flags = showFPS	? flags | 1 : flags & ~1; //bit 1
	flags = extraPackages ? flags | 2 : flags & ~2; //bit 2
	flags = barrier ? flags | 4 : flags & ~4; //bit 3
	flags = resetCounter ? flags | 8 : flags & ~8; //bit 4
	flags = stats ? flags | 16 : flags & ~16; //bit 5
	flags = takeScreenshot ? flags | 32 : flags & ~32; //bit 6
	flags = slowRendering ? flags | 64 : flags & ~64; //bit 7

	sgct::SharedData::Instance()->writeDouble(dt);
	sgct::SharedData::Instance()->writeString(mySharedString);
	sgct::SharedData::Instance()->writeDouble(curr_time);
	sgct::SharedData::Instance()->writeUChar(flags);
	sgct::SharedData::Instance()->writeObj<TestC>( myTestClass );

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			sgct::SharedData::Instance()->writeFloat( extraData[i] );
}

void myDecodeFun()
{
	dt = sgct::SharedData::Instance()->readDouble();
	mySharedString = sgct::SharedData::Instance()->readString();
	curr_time = sgct::SharedData::Instance()->readDouble();
	flags = sgct::SharedData::Instance()->readUChar();
	myTestClass = sgct::SharedData::Instance()->readObj<TestC>();

	showFPS	= flags & 0x0001;
	extraPackages = (flags>>1) & 0x0001;
	barrier = (flags>>2) & 0x0001;
	resetCounter = (flags>>3) & 0x0001;
	stats = (flags>>4) & 0x0001;
	takeScreenshot = (flags>>5) & 0x0001;
	slowRendering = (flags>>6) & 0x0001;

	if(extraPackages)
		for(int i=0;i<EXTENDED_SIZE;i++)
			extraData[i] = sgct::SharedData::Instance()->readFloat();
}

void drawGrid(float size, int steps)
{
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();
	glTranslatef(-size/2.0f, -size/2.0f, 0.0f);
	glColor4f(1.0f,1.0f,1.0f,0.5f);
	glLineWidth(1.0);

	glBegin(GL_LINES);
	//horizontal lines
	for( float f=0.0f; f<=size; f+= (size/static_cast<float>(steps)))
	{
		glVertex3f( 0.0f, f, 0.0f);
		glVertex3f( size, f, 0.0f);
	}
	//Vertical lines
	for( float f=0.0f; f<=size; f+= (size/static_cast<float>(steps)))
	{
		glVertex3f( f, 0.0f, 0.0f);
		glVertex3f( f, size, 0.0f);
	}
	glEnd();

	glVertex3f( -1.0f,  0.01f, 0.0f);
	glVertex3f(  1.0f,  0.01f, 0.0f);
	glVertex3f(  1.0f, -0.01f, 0.0f);
	glVertex3f( -1.0f, -0.01f, 0.0f);

	glPopMatrix();
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		static bool mousePointer = true;

		switch( key )
		{
		case 'I':
			if(action == GLFW_PRESS)
				showFPS = !showFPS;
			break;

		case 'E':
			if(action == GLFW_PRESS)
				extraPackages = !extraPackages;
			break;

		case 'B':
			if(action == GLFW_PRESS)
				barrier = !barrier;
			break;

		case 'R':
			if(action == GLFW_PRESS)
				resetCounter = true;
			break;

		case 'S':
			if(action == GLFW_PRESS)
				stats = !stats;
			break;

		case 'M':
			if(action == GLFW_PRESS)
			{
				mousePointer = !mousePointer; //toggle
				sgct::Engine::setMousePointerVisibility(mousePointer);
			}
			break;

		case GLFW_KEY_F9:
			if(action == GLFW_PRESS)
				slowRendering = !slowRendering;
			break;

		case GLFW_KEY_F10:
			if(action == GLFW_PRESS)
				takeScreenshot = true;
			break;

		case GLFW_KEY_UP:
			gEngine->getUserPtr()->setPos( gEngine->getUserPtr()->getPos() + glm::vec3(0.0f, 0.0f, 0.1f) );
			sgct::MessageHandler::Instance()->print("Up was pressed.\n");
			break;

		case GLFW_KEY_DOWN:
			gEngine->getUserPtr()->setPos( gEngine->getUserPtr()->getPos() - glm::vec3(0.0f, 0.0f, 0.1f) );
			sgct::MessageHandler::Instance()->print("Down was pressed.\n");
			break;
		}
	}
}

void externalControlCallback(const char * receivedChars, int size, int clientId)
{
	if( gEngine->isMaster() )
	{
		if(strcmp(receivedChars, "info") == 0)
			showFPS = !showFPS;
		else if(strcmp(receivedChars, "size") == 0)
			gEngine->setExternalControlBufferSize(4096);
	}
}
