#include "sgct.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();
//input callbacks
void keyCallback(int key, int action);
void mouseButtonCallback(int button, int action);

void drawXZGrid();
void drawPyramid(int index);
void createXZGrid(int size, float yPos);
void createPyramid(float width);

float rotationSpeed = 0.1f;
float walkingSpeed = 2.5f;

const int landscapeSize = 50;
const int numberOfPyramids = 150;

bool arrowButtons[4];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT };

//to check if left mouse button is pressed
bool mouseLeftButton = false;
/* Holds the difference in position between when the left mouse button
    is pressed and when the mouse button is held. */
double mouseDx = 0.0;
/* Stores the positions that will be compared to measure the difference. */
double mouseXPos[] = { 0.0, 0.0 };

glm::vec3 view(0.0f, 0.0f, 1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 pos(0.0f, 0.0f, 0.0f);

sgct::SharedObject<glm::mat4> xform;
glm::mat4 pyramidTransforms[numberOfPyramids];

enum geometryType { PYRAMID = 0, GRID };
GLuint VAOs[2] = { GL_FALSE, GL_FALSE };
GLuint VBOs[2] = { GL_FALSE, GL_FALSE };
//shader locations
GLint Matrix_Locs[2] = { -1, -1 };
GLint alpha_Loc = -1;

int numberOfVerts[2] = { 0, 0 };

class Vertex
{
public:
	Vertex() { mX = mY = mZ = 0.0f; }
	Vertex(float z, float y, float x) { mX = x; mY = y; mZ = z; }
	float mX, mY, mZ;
};

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setKeyboardCallbackFunction( keyCallback );
	gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );
	gEngine->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	gEngine->setCleanUpFunction( myCleanUpFun );

	for(int i=0; i<4; i++)
		arrowButtons[i] = false;

	if (!gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ))
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

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
	//generate the VAOs
	glGenVertexArrays(2, &VAOs[0]);
	//generate VBOs for vertex positions
	glGenBuffers(2, &VBOs[0]);

	createXZGrid(landscapeSize, -1.5f);
	createPyramid(0.6f);

	//pick a seed for the random function (must be same on all nodes)
	srand(9745);
	for(int i=0; i<numberOfPyramids; i++)
	{
		float xPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);
		float zPos = static_cast<float>(rand()%landscapeSize - landscapeSize/2);

		pyramidTransforms[i] = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, -1.5f, zPos));
	}

	sgct::ShaderManager::instance()->addShaderProgram("gridShader",
		"gridShader.vert",
		"gridShader.frag");
	sgct::ShaderManager::instance()->bindShaderProgram("gridShader");
	Matrix_Locs[GRID] = sgct::ShaderManager::instance()->getShaderProgram("gridShader").getUniformLocation("MVP");
	sgct::ShaderManager::instance()->unBindShaderProgram();

	sgct::ShaderManager::instance()->addShaderProgram("pyramidShader",
		"pyramidShader.vert",
		"pyramidShader.frag");
	sgct::ShaderManager::instance()->bindShaderProgram("pyramidShader");
	Matrix_Locs[PYRAMID] = sgct::ShaderManager::instance()->getShaderProgram("pyramidShader").getUniformLocation("MVP");
	alpha_Loc = sgct::ShaderManager::instance()->getShaderProgram("pyramidShader").getUniformLocation("alpha");
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		if( mouseLeftButton )
		{
			double tmpYPos;
			//get the mouse pos from first window
			sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[0], &tmpYPos );
			mouseDx = mouseXPos[0] - mouseXPos[1];
		}
		else
		{
			mouseDx = 0.0;
		}

		static float panRot = 0.0f;
		panRot += (static_cast<float>(mouseDx) * rotationSpeed * static_cast<float>(gEngine->getDt()));

		glm::mat4 ViewRotateX = glm::rotate(
			glm::mat4(1.0f),
			panRot,
			glm::vec3(0.0f, 1.0f, 0.0f)); //rotation around the y-axis

		view = glm::inverse(glm::mat3(ViewRotateX)) * glm::vec3(0.0f, 0.0f, 1.0f);

		glm::vec3 right = glm::cross(view, up);

		if( arrowButtons[FORWARD] )
			pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
		if( arrowButtons[BACKWARD] )
			pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * view);
		if( arrowButtons[LEFT] )
			pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);
		if( arrowButtons[RIGHT] )
			pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);

		/*
			To get a first person camera, the world needs
			to be transformed around the users head.

			This is done by:
			1, Transform the user to coordinate system origin
			2, Apply navigation
			3, Apply rotation
			4, Transform the user back to original position

			However, mathwise this process need to be reversed
			due to the matrix multiplication order.
		*/

		glm::mat4 result;
		//4. transform user back to original position
		result = glm::translate(glm::mat4(1.0f), sgct::Engine::getDefaultUserPtr()->getPos());
		//3. apply view rotation
		result *= ViewRotateX;
		//2. apply navigation translation
		result *= glm::translate(glm::mat4(1.0f), pos);
		//1. transform user to coordinate system origin
		result *= glm::translate(glm::mat4(1.0f), -sgct::Engine::getDefaultUserPtr()->getPos());

		xform.setVal( result );
	}
}

void myDrawFun()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);

	drawXZGrid();

	for (int i = 0; i < numberOfPyramids; i++)
		drawPyramid(i);

	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeObj( &xform );
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readObj( &xform );
}

void keyCallback(int key, int action)
{
	if( gEngine->isMaster() )
	{
		switch( key )
		{
		case SGCT_KEY_UP:
		case SGCT_KEY_W:
			arrowButtons[FORWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;

		case SGCT_KEY_DOWN:
		case SGCT_KEY_S:
			arrowButtons[BACKWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;

		case SGCT_KEY_LEFT:
		case SGCT_KEY_A:
			arrowButtons[LEFT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
			break;

		case SGCT_KEY_RIGHT:
		case SGCT_KEY_D:
			arrowButtons[RIGHT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false);
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
			mouseLeftButton = (action == SGCT_PRESS ? true : false);
			double tmpYPos;
			//set refPos
			sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[1], &tmpYPos );
			break;
		}
	}
}

void drawXZGrid(void)
{
	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * xform.getVal();

	sgct::ShaderManager::instance()->bindShaderProgram("gridShader");

	glUniformMatrix4fv(Matrix_Locs[GRID], 1, GL_FALSE, &MVP[0][0]);

	glBindVertexArray(VAOs[GRID]);

	glLineWidth(3.0f);
	glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
	glDrawArrays(GL_LINES, 0, numberOfVerts[GRID]);

	//unbind
	glBindVertexArray(0);
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void drawPyramid(int index)
{
	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * xform.getVal() * pyramidTransforms[index];

	sgct::ShaderManager::instance()->bindShaderProgram("pyramidShader");

	glUniformMatrix4fv(Matrix_Locs[PYRAMID], 1, GL_FALSE, &MVP[0][0]);

	glBindVertexArray(VAOs[PYRAMID]);

	//draw lines
	glLineWidth(2.0f);
	glPolygonOffset(1.0f, 0.1f); //offset to avoid z-buffer fighting
	glUniform1f(alpha_Loc, 0.8f);
	glDrawArrays(GL_LINES, 0, 16);
	//draw triangles
	glPolygonOffset(0.0f, 0.0f); //offset to avoid z-buffer fighting
	glUniform1f(alpha_Loc, 0.3f);
	glDrawArrays(GL_TRIANGLES, 16, 12);

	//unbind
	glBindVertexArray(0);
	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void createXZGrid(int size, float yPos)
{
	numberOfVerts[GRID] = size * 4;
	Vertex * vertData = new (std::nothrow) Vertex[numberOfVerts[GRID]];
	
	int i = 0;
	for (int x = -(size / 2); x < (size / 2); x++)
	{
		vertData[i].mX = static_cast<float>(x);
		vertData[i].mY = yPos;
		vertData[i].mZ = static_cast<float>(-(size / 2));

		vertData[i + 1].mX = static_cast<float>(x);
		vertData[i + 1].mY = yPos;
		vertData[i + 1].mZ = static_cast<float>(size / 2);

		i += 2;
	}

	for (int z = -(size / 2); z < (size / 2); z++)
	{
		vertData[i].mX = static_cast<float>(-(size / 2));
		vertData[i].mY = yPos;
		vertData[i].mZ = static_cast<float>(z);

		vertData[i + 1].mX = static_cast<float>(size / 2);
		vertData[i + 1].mY = yPos;
		vertData[i + 1].mZ = static_cast<float>(z);

		i += 2;
	}

	glBindVertexArray(VAOs[GRID]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[GRID]);
	
	//upload data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberOfVerts[GRID], vertData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		reinterpret_cast<void*>(0) // array buffer offset
		);

	//unbind
	glBindVertexArray(GL_FALSE);
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

	//clean up
	delete[] vertData;
	vertData = NULL;
}

void createPyramid(float width)
{
	std::vector<Vertex> vertData;

	//enhance the pyramids with lines in the edges
	//-x
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	//+x
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	//-z
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	//+z
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	
	//triangles
	//-x
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	//+x
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	//-z
	vertData.push_back(Vertex(width / 2.0f, 0.0f, -width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, -width / 2.0f));
	//+z
	vertData.push_back(Vertex(-width / 2.0f, 0.0f, width / 2.0f));
	vertData.push_back(Vertex(0.0f, 2.0f, 0.0f));
	vertData.push_back(Vertex(width / 2.0f, 0.0f, width / 2.0f));

	numberOfVerts[PYRAMID] = static_cast<int>( vertData.size() );

	glBindVertexArray(VAOs[PYRAMID]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[PYRAMID]);

	//upload data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*numberOfVerts[PYRAMID], &vertData[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		reinterpret_cast<void*>(0) // array buffer offset
		);

	//unbind
	glBindVertexArray(GL_FALSE);
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

	//clean up
	vertData.clear();
}

void myCleanUpFun()
{
	if (VBOs[0])
		glDeleteBuffers(2, &VBOs[0]);
	if (VAOs[0])
		glDeleteVertexArrays(2, &VAOs[0]);
}
