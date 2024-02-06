/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "sgct.h"

#ifdef __APPLE__
#include <OpenAL/al.h>
// #include <ALUT/alut.h>
#else
#include <AL/al.h>
#endif
#include <AL/alut.h>

sgct::Engine* gEngine;

// callbacks
void myInitOGLFun();
void myDrawFun();
void myPreSyncFun();
void myPostSyncPreDrawFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

// other functions
void setAudioSource(ALuint& buffer, ALuint& source, const char* filename);

// open AL data
ALuint audio_buffer0 = AL_NONE;
ALuint source0;
glm::vec4 audioPos;

// other variables
sgct::SharedDouble curr_time(0.0);
float speed = 25.f;
float radius = 7.4f;
float objectRadius = 5.f;
float PI = 3.141592654f;
float domeTilt = 27.f;

sgct_utils::SGCTSphere* sphere;
sgct_utils::SGCTDome* dome;

int main(int argc, char* argv[]) {
    gEngine = new sgct::Engine(argc, argv);

    // Bind your functions
    gEngine->setInitOGLFunction(myInitOGLFun);
    gEngine->setDrawFunction(myDrawFun);
    gEngine->setPreSyncFunction(myPreSyncFun);
    gEngine->setPostSyncPreDrawFunction(myPostSyncPreDrawFun);
    gEngine->setCleanUpFunction(myCleanUpFun);
    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    // Main loop
    gEngine->render();

    // Clean up
    delete gEngine;

    // Exit program
    exit(EXIT_SUCCESS);
}

void myInitOGLFun() {
    glEnable(GL_DEPTH_TEST);

    sphere = new sgct_utils::SGCTSphere(0.5f, 8);
    dome = new sgct_utils::SGCTDome(7.4f, 165.f, 36, 10);

    alutInit(NULL, 0);

    // Check for errors if any
    sgct::MessageHandler::printInfo("ALUT init: %s", alutGetErrorString(alutGetError()));

    setAudioSource(audio_buffer0, source0, "file1.wav");

    glm::vec3 userPos = gEngine->getDefaultUserPtr()->getPos();
    alListener3f(AL_POSITION, userPos.x, userPos.y, userPos.z);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);

    alSourcef(source0, AL_PITCH, 1.f);
    alSourcef(source0, AL_GAIN, 1.f);
    alSource3f(source0, AL_POSITION, 0.f, 0.f, 0.f);
    alSource3f(source0, AL_VELOCITY, 0.f, 0.f, 0.f);
    alSourcei(source0, AL_LOOPING, AL_TRUE);

    alSourcePlay(source0);
}

void myPostSyncPreDrawFun() {
    float angle = glm::radians(static_cast<float>(curr_time.getVal()) * speed);
    glm::vec4 p;
    p.x = objectRadius * std::sin(angle);
    p.z = objectRadius * std::cos(angle);
    p.y = 2.f;

    glm::mat4 rotMat = glm::mat4(1.f);
    rotMat = glm::rotate(rotMat, -domeTilt, glm::vec3(1.f, 0.f, 0.f));
    audioPos = rotMat * p;

    alSource3f(source0, AL_POSITION, audioPos.x, audioPos.y, audioPos.z);
}

void myDrawFun() {
    glLineWidth(2.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(audioPos.x, audioPos.y, audioPos.z);
    glColor4f(1.f, 0.4f, 0.1f, 0.8f);
    //wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    sphere->draw();
    //reset to normal rendering
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();

    glPushMatrix();
    glColor4f(0.f, 0.4f, 1.f, 0.8f);
    dome->draw();
    glPopMatrix();

    glDisable(GL_BLEND);
}

void myPreSyncFun() {
    //set the time only on the master
    if (gEngine->isMaster()) {
        //get the time in seconds
        curr_time.setVal(sgct::Engine::getTime());
    }
}

void myEncodeFun() {
    sgct::SharedData::instance()->writeDouble(&curr_time);
}

void myDecodeFun() {
    sgct::SharedData::instance()->readDouble(&curr_time);
}

void myCleanUpFun() {
    alDeleteSources(1, &source0);
    alDeleteBuffers(1, &audio_buffer0);
    alutExit();

    delete sphere;
    delete dome;
}

void setAudioSource(ALuint& buffer, ALuint& source, const char* filename) {
    alGenBuffers(1, &buffer);
    alGenSources(1, &source);

    buffer = alutCreateBufferFromFile(filename);
    if (buffer == AL_NONE) {
        sgct::MessageHandler::printInfo(
            "Failed to read audio file '%s', error: %s",
            filename, alutGetErrorString(alutGetError())
        );
    }
    alSourcei(source, AL_BUFFER, buffer);
}
