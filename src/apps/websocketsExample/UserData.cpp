#include "UserData.hpp"
#include <stdio.h> //printf
#include <math.h>

#define CANVAS_SIZE 500.0f
#define DEG90_TO_RAD 1.570796327f
#define DEG180_TO_RAD 3.141592654f

UserData::UserData()
{
    mTheta = 0.0f;
    mPhi = DEG180_TO_RAD;
    //mColor = RED;
    mRed = 1.0f;
    mGreen = 0.0f;
    mBlue = 0.0f;
    mTimeStamp = -1.0f;
}

UserData::~UserData()
{
}

void UserData::set(float theta, float phi, float r, float g, float b, float timeStamp)
{
    mTheta = theta;
    mPhi = phi;
    //mColor = color;
    mTimeStamp = timeStamp;
    mRed = r;
    mGreen = g;
    mBlue = b;
}

void UserData::setCartesian2d(int x, int y, float r, float g, float b, float timeStamp)
{
    float s = (static_cast<float>(x)/CANVAS_SIZE)*2.0f - 1.0f;
    float t = (static_cast<float>(y)/CANVAS_SIZE)*2.0f - 1.0f;
    
    float r2 = s*s + t*t;
    if( r2 <= 1.0f )
    {
        mPhi = sqrt(r2) * DEG90_TO_RAD;
        mTheta = atan2f(s,t);
    }
    
    //fprintf(stderr, "phi: %f theta: %f\n", mPhi, mTheta);
    
    //mColor = color;
    mRed = r;
    mGreen = g;
    mBlue = b;
    mTimeStamp = timeStamp;
}
