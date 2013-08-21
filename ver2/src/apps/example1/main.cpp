/*
 * sgct_main.cpp
 *
 *  Created on: Jun 12, 2013
 *      Author: jonathan
 */

#include <cstdlib>
#include <iostream>
#include "sgct.h"

#define NO_FACES 5

using namespace sgct;
using namespace glm;
using namespace std;

#if _MSC_VER
#define snprintf _snprintf
#endif

static Engine * gEngine;

static float rotations[NO_FACES][3] = {
		{ 0.0, 2.189, 0.039 },
		{ 91.647, 2.279, -0.432 },
		{ -179.341, 2.38, -0.006 },
		{ -88.75, 0.328, 1.187 },
		{ 1.559, 88.064, 0.044 }
};

static float translations[NO_FACES][3] = {
		{ -0.06, 0.0, -1.0 },
		{ 0.0, -0.06, -1.0 },
		{ 0.0,  0.0, -1.1 },
		{ 0.09, 0.0, -1.0 },
		{ 0.0,  0.0, -1.0 }
};
static float scales[NO_FACES][3] = {
		{ 1.0, 1.0, 0.0 },
		{ 1.0, 1.0, 0.0 },
		{ 1.0, 1.0, 0.0 },
		{ 1.0, 1.0, 0.0 },
		{ 1.0, 1.0, 0.0 }
};

static unsigned int shader_id;
static unsigned int mvp_location;
static unsigned int vertex_location;
static unsigned int tex_coord_location;

static void draw();
static void init();
static void cleanup();
static int shader_load(unsigned int * id, const char * name);
static char * read_text_file(const char * path);

int main() {
	{
		int argc = 2;
		const char * argv[2] = { "-config", "fisheye.xml" };
		char ** argv_tmp = (char **)argv;

		gEngine = new Engine(argc, argv_tmp);
	}

	// Bind your functions
	gEngine->setInitOGLFunction(init);
	gEngine->setDrawFunction(draw);
	gEngine->setCleanUpFunction(cleanup);

	// Init the engine
	if (!gEngine->init(sgct::Engine::OpenGL_Compablity_Profile)) {
		return 0;
	}

	// Main loop
	gEngine->render();

	// Clean up (de-allocate)
	delete gEngine;

	return 0;
}

static void draw() {

	if (gEngine->checkForOGLErrors()) {
		cerr << "OpenGL errors before draw" << endl;
	}

	glEnable(GL_TEXTURE_2D);

	mat4 mvp, m;

	glUseProgram(shader_id);

	static const vec3 yawAxis(0.0f, 1.0f, 0.0f);
	static const vec3 pitchAxis(1.0f, 0.0f, 0.0f);
	static const vec3 rollAxis(0.0f, 0.0f,-1.0f);

	const mat4 & pv = gEngine->getActiveViewProjectionMatrix();

	for (int i = 0; i < NO_FACES; ++i) {
		m = mat4(1.0f);

		m = rotate(m, rotations[i][2], rollAxis);
		m = rotate(m, rotations[i][1], pitchAxis);
		m = rotate(m, rotations[i][0], yawAxis);
		m = translate(m,
				vec3(
						translations[i][0],
						translations[i][1],
						translations[i][2]));
		m = scale(m,
				vec3(
						scales[i][0],
						scales[i][1],
						scales[i][2]));

		mvp = pv * m;

		glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

		glBegin(GL_QUADS);
		glVertexAttrib2f(tex_coord_location,  1.5f,-0.5f); glVertexAttrib3f(vertex_location, 2.0f,  2.0f, 0.0f);
		glVertexAttrib2f(tex_coord_location,  1.5f, 1.5f); glVertexAttrib3f(vertex_location, 2.0f, -2.0f, 0.0f);
		glVertexAttrib2f(tex_coord_location, -0.5f, 1.5f); glVertexAttrib3f(vertex_location,-2.0f, -2.0f, 0.0f);
		glVertexAttrib2f(tex_coord_location, -0.5f,-0.5f); glVertexAttrib3f(vertex_location,-2.0f,  2.0f, 0.0f);
		glEnd();
	}

	glUseProgram(0);

	glDisable(GL_TEXTURE_2D);

	if (gEngine->checkForOGLErrors()) {
		cerr << "OpenGL errors after draw" << endl;
	}
}

static void init() {

	if (gEngine->checkForOGLErrors()) {
		cerr << "OpenGL errors before init" << endl;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	shader_load(&shader_id, "dome");
	glUseProgram(shader_id);
	vertex_location = glGetAttribLocation(shader_id, "vertex_pos");
	tex_coord_location = glGetAttribLocation(shader_id, "tex_coord");
	mvp_location = glGetUniformLocation(shader_id, "mvp");

	if (gEngine->checkForOGLErrors()) {
		cerr << "OpenGL errors after init" << endl;
	}
}

static void cleanup() {
	glDeleteProgram(shader_id);
}

static int shader_load(unsigned int * id, const char * name) {

	unsigned int shader_id;
	unsigned int vertex_id;
	unsigned int fragment_id;
	char fragment_path[512];
	char vartex_path[512];

	snprintf(fragment_path, 512, "%s.frag", name);
	snprintf(vartex_path, 512, "%s.vert", name);

	char * fragment_source = read_text_file(fragment_path);
	char * vertex_source = read_text_file(vartex_path);

	fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
	vertex_id = glCreateShader(GL_VERTEX_SHADER);
	shader_id = glCreateProgram();

	glShaderSource(fragment_id, 1, (const char **)&fragment_source, NULL);
	glShaderSource(vertex_id, 1, (const char **)&vertex_source, NULL);

	glCompileShader(fragment_id);
	glCompileShader(vertex_id);

	glAttachShader(shader_id, fragment_id);
	glAttachShader(shader_id, vertex_id);

	glLinkProgram(shader_id);

	free(fragment_source);
	fragment_source = NULL;
	free(vertex_source);
	vertex_source = NULL;

	glDeleteShader(fragment_id);
	glDeleteShader(vertex_id);

	*id = shader_id;

	return 0;
}

static char * read_text_file(const char * path) {
	FILE * file;
	char * content = NULL;

	int count = 0;

	file = fopen(path, "r");

	fseek(file, 0, SEEK_END);
	count = (int)ftell(file);
	rewind(file);

	content = (char *)malloc((size_t)(count + 1));

	count = (int)fread(content, 1, (size_t)count, file);
	content[count] = '\0';

	fclose(file);

	return content;
}
