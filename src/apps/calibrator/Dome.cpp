#include "Dome.h"

#include "sgct.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <glm/gtc/type_ptr.hpp>

#define EXPORT_DOME_MODEL 0


Dome::Dome(float radius, float tilt)
    : mRadius(radius)
    , mTilt(tilt)
{}

Dome::~Dome() {
    glDeleteLists(mGeoDisplayList, 4);
}

void Dome::drawGeoCorrPattern() {
    glCallList(mGeoDisplayList);
}

void Dome::drawChannelZones() {
    glCallList(mChannelZonesDisplayList);
}

void Dome::drawBlendZones() {
    glCallList(mBlendZonesDisplayList);
}

void Dome::drawTexturedSphere() {
    glPushMatrix();
    glRotatef(-(mTilt+mTiltOffset), 1.0f, 0.0f, 0.0f);
    glCallList( mTexDisplayList );
    glPopMatrix();
}

void Dome::drawColCorrPattern(glm::vec3* color, PatternMode mode) {
    constexpr const int ElevationSteps = 32;
    constexpr const int AzimuthSteps = 128;
    
    glPushMatrix();
    glRotatef(-mTilt, 1.0f, 0.0f, 0.0f);

    float i0;
    float i1;

    for (int e = 0; e < (ElevationSteps - 1); e++) {
        float elevation0 = glm::radians(
            static_cast<float>(e * 90) / static_cast<float>(ElevationSteps)
        );
        float elevation1 = glm::radians(
            static_cast<float>((e + 1) * 90) / static_cast<float>(ElevationSteps)
        );
        
        switch (mode) {
            case PatternMode::Step:
                i0 = 1.f - static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
                i1 = i0;
                break;
            case PatternMode::StepInverted:
                i0 = static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
                i1 = i0;
                break;
            case PatternMode::Gradient:
                i0 = 1.f - static_cast<float>(e) / static_cast<float>(ElevationSteps - 2);
                i1 = 1.f - static_cast<float>(e + 1) /
                           static_cast<float>(ElevationSteps - 2);
                break;
            case PatternMode::GradientInverted:
                i0 = static_cast<float>(e) / static_cast<float>(ElevationSteps - 2);
                i1 = static_cast<float>(e + 1) / static_cast<float>(ElevationSteps - 2);
                break;
            case PatternMode::Solid:
            default:
                i0 = 1.f;
                i1 = i0;
                break;
        }

        glBegin(GL_TRIANGLE_STRIP);

        const float y0 = mRadius * sin(elevation0);
        const float y1 = mRadius * sin(elevation1);
            
        for (int a = 0; a <= AzimuthSteps; a++) {
            const float azimuth = glm::radians(
                static_cast<float>(a * 360) / static_cast<float>(AzimuthSteps)
            );
            
            const float x0 = mRadius * cos(elevation0) * sin(azimuth);
            const float z0 = -mRadius * cos(elevation0) * cos(azimuth);

            const float x1 = mRadius * cos(elevation1) * sin(azimuth);
            const float z1 = -mRadius * cos(elevation1) * cos(azimuth);

            glColor3f(color->r * i0, color->g * i0,  color->b * i0);
            glVertex3f(x0, y0, z0);
            glColor3f(color->r * i1, color->g * i1,  color->b * i1);
            glVertex3f(x1, y1, z1);
        }

        glEnd();
    }
    
    //CAP
    int e = ElevationSteps - 1;
    float elevation0 = glm::radians(
        static_cast<float>(e * 90) / static_cast<float>(ElevationSteps)
    );
    float elevation1 = glm::radians(
        static_cast<float>((e + 1) * 90) / static_cast<float>(ElevationSteps)
    );

    glBegin(GL_TRIANGLE_FAN);

    const float y0 = mRadius * sin(elevation0);
    const float y1 = mRadius * sin(elevation1);

    glColor3f(color->r * i1, color->g * i1, color->b * i1);
    glVertex3f(0.f, y1, 0.f);
        
    for (int a = 0; a <= AzimuthSteps; a++) {
        const float azimuth = glm::radians(
            static_cast<float>(a * 360) / static_cast<float>(AzimuthSteps)
        );
        
        const float x0 = mRadius * cos(elevation0) * sin(azimuth);
        const float z0 = -mRadius * cos(elevation0) * cos(azimuth);

        glColor3f(color->r * i0, color->g * i0, color->b * i0);
        glVertex3f(x0, y0, z0);
    }

    glEnd();

    glPopMatrix();
}

void Dome::generateDisplayList() {
    mGeoDisplayList = glGenLists(4);
    mBlendZonesDisplayList = mGeoDisplayList+1;
    mChannelZonesDisplayList = mGeoDisplayList+2;
    mTexDisplayList = mGeoDisplayList+3;

    glNewList(mGeoDisplayList, GL_COMPILE);
    
    glPushMatrix();
    glRotatef(-mTilt, 1.f, 0.f, 0.f);

    //float x, y, z;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLineWidth(2.0f);

    for (float e = 0.f; e <= 90.f; e += 2.25f) {
        const float elevation = glm::radians(e);
    
        //nine degrees
        if (static_cast<int>(e) % 9 == 0) {
            glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
        }
        else {
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        }
        
        glBegin(GL_LINE_LOOP);

        const float y = mRadius * sin(elevation);
        
        for (float a = 0.f; a < 360.f; a += 2.25f) {
            const float azimuth = glm::radians(a);
            const float x = mRadius * cos(elevation) * sin(azimuth);
            const float z = -mRadius * cos(elevation) * cos(azimuth);
            
            glVertex3f(x, y, z);
        }

        glEnd();
    }

    for (float a = 0.f; a < 360.f; a += 2.25f) {
        const float azimuth = glm::radians(a);
        if (static_cast<int>(a) % 45 == 0) {
            glColor4f(0.f, 1.f, 1.f, 0.5f);
        }
        else if (static_cast<int>(a) % 9 == 0) {
            glColor4f(1.f, 1.f, 0.f, 0.5f);
        }
        else {
            glColor4f(1.f, 1.f, 1.f, 0.5f);
        }

        
        glBegin(GL_LINE_STRIP);
        for (float e = 0.f; e <= 90.f; e += 2.25f) {
            const float elevation = glm::radians(e);
            const float x = mRadius * cos(elevation) * sin(azimuth);
            const float y = mRadius * sin(elevation);
            const float z = -mRadius * cos(elevation) * cos(azimuth);
                
            glVertex3f(x, y, z);
        }

        glEnd();
    }

    glDisable(GL_BLEND);
    glPopMatrix();
    glEndList();

    glNewList(mBlendZonesDisplayList, GL_COMPILE);
    
    glPushMatrix();
    glRotatef(-mTilt, 1.0f, 0.0f, 0.0f);
    drawLongitudeLines(-180.f, 8.25f, 90.f, 128);
    drawLongitudeLines(72.f - 180.f, 8.25f, 90.f, 128);
    drawLongitudeLines(144.f - 180.f, 8.25f, 90.f, 128);
    drawLongitudeLines(-144.f - 180.f, 8.25f, 90.f, 128);
    drawLongitudeLines(-72.f - 180.f, 8.25f, 90.f, 128);
    drawLongitudeLines(-2.75f - 180.f, 8.25f, 90.f, 128);

    // Cap blend area
    drawLatitudeLines(8.25f + 49.5f, 0.f, 360.f, 256);
    drawLatitudeLines(8.25f + 56.f, 0.f, 360.f, 256);

    // Cap-Ch1 blend
    /*drawVerticalFrustumLine(gmtl::Vec3f(-5.18765f, 0.852762f, -5.88139f),
        gmtl::Vec3f(-5.18765f, 5.939094f, -2.23591f),
        64);*/


    // Side blends
    drawLongitudeLines(-180.f - 6.35f, 0.f, 8.25f + 56.0f, 64);
    drawLongitudeLines(-180.f + 6.35f, 0.f, 8.25f + 56.0f, 64);

    drawLongitudeLines(72.f - 180.f - 6.35f, 0.f, 8.25f + 56.f, 64);
    drawLongitudeLines(72.f - 180.f + 6.35f, 0.f, 8.25f + 56.f, 64);

    drawLongitudeLines(144.f - 180.f - 6.35f, 0.f, 8.25f + 56.f, 64);
    drawLongitudeLines(144.f - 180.f + 6.35f, 0.f, 8.25f + 56.f, 64);

    drawLongitudeLines(-144.f - 180.f - 6.35f, 0.f, 8.25f + 56.f, 64);
    drawLongitudeLines(-144.f - 180.f + 6.35f, 0.f, 8.25f + 56.f, 64);

    drawLongitudeLines(-72.f - 180.f - 6.35f, 0.f, 8.25f + 56.f, 64);
    drawLongitudeLines(-72.f - 180.f + 6.35f, 0.f, 8.25f + 56.f, 64);

    // draw dome bottom
    drawLatitudeLines(8.25f, 0.0f, 360.0f, 256);

    glPopMatrix();
    glEndList();

    glNewList(mChannelZonesDisplayList, GL_COMPILE);
    
    glPushMatrix();
    glRotatef(-mTilt, 1.f, 0.f, 0.f);
    // Draw channel edges
    // CH 1
    // Left
    drawVerticalFrustumLine(
        glm::vec3(-5.18765f, 0.852762f, -5.88139f),
        glm::vec3(-5.18765f, 5.939094f, -2.23591f),
        64
    );
    // Right
    drawVerticalFrustumLine(
        glm::vec3(5.18765f, 0.852762f, -5.88139f),
        glm::vec3(5.18765f, 5.939094f, -2.23591f),
        64
    );
    // Bottom
    drawVerticalFrustumLine(
        glm::vec3(-5.18765f, 0.852762f, -5.88139f),
        glm::vec3(5.18765f, 0.852762f, -5.88139f),
        64
    );
    // Top
    drawVerticalFrustumLine(
        glm::vec3(-5.18765f, 5.939094f, -2.23591f),
        glm::vec3(5.18765f, 5.939094f, -2.23591f),
        64
    );

    // CH 2
    // Left
    drawVerticalFrustumLine(
        glm::vec3(3.99047f, 0.85276f, -6.75120f),
        glm::vec3(0.52340f, 5.93909f, -5.62468f),
        64
    );
    // Right
    drawVerticalFrustumLine(
        glm::vec3(7.19661f, 0.85276f, 3.11630f),
        glm::vec3(3.72955f, 5.93909f, 4.24281f),
        64
    );
    // Bottom
    drawVerticalFrustumLine(
        glm::vec3(7.19661f, 0.85276f, 3.11630f),
        glm::vec3(3.99047f, 0.85276f, -6.75120f),
        64
    );
    // Top
    drawVerticalFrustumLine(
        glm::vec3(0.52340f, 5.93909f, -5.62468f),
        glm::vec3(3.72955f, 5.93909f, 4.24281f),
        64
    );

    // CH 3
    // Left
    drawVerticalFrustumLine(
        glm::vec3(7.65389f, 0.85276f, 1.70892f),
        glm::vec3(5.51113f, 5.93909f, -1.24034f),
        64
    );
    // Right
    drawVerticalFrustumLine(
        glm::vec3(-0.73990f, 0.85276f, 7.80737f),
        glm::vec3(-2.88266f, 5.93909f, 4.85811f),
        64
    );
    // Bottom
    drawVerticalFrustumLine(
        glm::vec3(7.65389f, 0.85276f, 1.70892f),
        glm::vec3(-0.73990f, 0.85276f, 7.80737f),
        64
    );
    // Top
    drawVerticalFrustumLine(
        glm::vec3(-2.88266f, 5.93909f, 4.85811f),
        glm::vec3(5.51113f, 5.93909f, -1.24034f),
        64
    );

    // CH 4
    // Left
    drawVerticalFrustumLine(
        glm::vec3(0.73990f, 0.85276f, 7.80737f),
        glm::vec3(2.88266f, 5.93909f, 4.85811f),
        64
    );
    // Right
    drawVerticalFrustumLine(
        glm::vec3(-7.65389f, 0.85276f, 1.70892f),
        glm::vec3(-5.51113f, 5.93909f, -1.24034f),
        64
    );
    // Bottom
    drawVerticalFrustumLine(
        glm::vec3(0.73990f, 0.85276f, 7.80737f),
        glm::vec3(-7.65389f, 0.85276f, 1.70892f),
        64
    );
    // Top
    drawVerticalFrustumLine(
        glm::vec3(-5.51113f, 5.93909f, -1.24034f),
        glm::vec3(2.88266f, 5.93909f, 4.85811f),
        64
    );

    // CH 5
    // Left
    drawVerticalFrustumLine(
        glm::vec3(-7.19661f, 0.85276f, 3.11630f),
        glm::vec3(-3.72955f, 5.93909f, 4.24281f),
        64
    );
    // Right
    drawVerticalFrustumLine(
        glm::vec3(-3.99047f, 0.85276f, -6.75120f),
        glm::vec3(-0.52340f, 5.93909f, -5.62468f),
        64
    );
    // Bottom
    drawVerticalFrustumLine(
        glm::vec3(-7.19661f, 0.85276f, 3.11630f),
        glm::vec3(-3.99047f, 0.85276f, -6.75120f),
        64
    );
    // Top
    drawVerticalFrustumLine(
        glm::vec3(-0.52340f, 5.93909f, -5.62468f),
        glm::vec3(-3.72955f, 5.93909f, 4.24281f),
        64
    );

    // CH 6
    // Left
    drawVerticalFrustumLine(
        glm::vec3(-4.04754f, 6.30954f, -3.67653f),
        glm::vec3(-3.67653f, 6.30954f, 4.04754f),
        64
    );
    // Right
    drawVerticalFrustumLine(
        glm::vec3(3.67653f, 6.30954f, -4.04754f),
        glm::vec3(4.04754f, 6.30954f, 3.67653f),
        64
    );
    // Bottom
    drawVerticalFrustumLine(
        glm::vec3(-4.04754f, 6.30954f, -3.67653f),
        glm::vec3(3.67653f, 6.30954f, -4.04754f),
        64
    );
    // Top
    drawVerticalFrustumLine(
        glm::vec3(4.04754f, 6.30954f, 3.67653f),
        glm::vec3(-3.67653f, 6.30954f, 4.04754f),
        64
    );
    glPopMatrix();
    glEndList();
    
    glNewList(mTexDisplayList, GL_COMPILE);
    glColor4f(1.f, 1.f, 1.f, 1.f);

    DomeVertex dv0, dv1;

    constexpr const float Lift = 7.5f;

    constexpr const int ElevationSteps = 64;
    constexpr const int AzimuthSteps = 256;

    std::vector<DomeVertex> mRingVertices;
    for (int e = 0; e < (ElevationSteps -1 ); e++) {
        mRingVertices.clear();
        
        const float de0 = static_cast<float>(e) / static_cast<float>(ElevationSteps);
        const float elevation0 = glm::radians(Lift + de0 * (90.f - Lift));
        
        const float de1 = static_cast<float>(e+1) / static_cast<float>(ElevationSteps);
        const float elevation1 = glm::radians(Lift + de1 * (90.f - Lift));
            
        glBegin(GL_TRIANGLE_STRIP);

        dv0.y = sin(elevation0);
        dv1.y = sin(elevation1);

        for (int a = 0; a <= AzimuthSteps; a++) {
            const float azimuth = glm::radians(
                static_cast<float>(a * 360) / static_cast<float>(AzimuthSteps)
            );
                
            dv0.x = cos(elevation0) * sin(azimuth);
            dv0.z = -cos(elevation0) * cos(azimuth);

            dv1.x = cos(elevation1) * sin(azimuth);
            dv1.z = -cos(elevation1) * cos(azimuth);

            dv0.s = (static_cast<float>(ElevationSteps - e ) /
                    static_cast<float>(ElevationSteps)) * sin(azimuth);
            dv1.s = (static_cast<float>(ElevationSteps - (e + 1)) /
                    static_cast<float>(ElevationSteps)) * sin(azimuth);
            dv0.t = (static_cast<float>(ElevationSteps - e ) /
                    static_cast<float>(ElevationSteps)) * -cos(azimuth);
            dv1.t = (static_cast<float>(ElevationSteps - (e + 1)) /
                    static_cast<float>(ElevationSteps)) * -cos(azimuth);

            dv0.s = (dv0.s * 0.5f) + 0.5f;
            dv1.s = (dv1.s * 0.5f) + 0.5f;
            dv0.t = (dv0.t * 0.5f) + 0.5f;
            dv1.t = (dv1.t * 0.5f) + 0.5f;

            glMultiTexCoord2f(GL_TEXTURE0, dv0.s, dv0.t);
            glMultiTexCoord2f(GL_TEXTURE1, dv0.s, dv0.t);
            glVertex3f(dv0.x * mRadius, dv0.y * mRadius, dv0.z * mRadius);

            glMultiTexCoord2f(GL_TEXTURE0, dv1.s, dv1.t);
            glMultiTexCoord2f(GL_TEXTURE1, dv1.s, dv1.t);
            glVertex3f(dv1.x * mRadius, dv1.y * mRadius, dv1.z * mRadius);

            mRingVertices.push_back(dv0);
            //mRingVertices.push_back(dv1);
        }

        glEnd();
        mVertices.push_back(mRingVertices);
    }
        
    // CAP
    DomeVertex pole;
    mRingVertices.clear();

    int e = ElevationSteps - 1;
    const float de0 = static_cast<float>(e) / static_cast<float>(ElevationSteps);
    const float elevation0 = glm::radians(Lift + de0 * (90.f - Lift));

    const float de1 = static_cast<float>(e + 1) / static_cast<float>(ElevationSteps);
    const float elevation1 = glm::radians(Lift + de1 * (90.f - Lift));

    glBegin(GL_TRIANGLE_FAN);

    dv1.x = 0.f;
    dv1.z = 0.f;

    dv0.y = sin(elevation0);
    dv1.y = sin(elevation1);

    dv1.s = 0.5f;
    dv1.t = 0.5f;

    glMultiTexCoord2f(GL_TEXTURE0, dv1.s, dv1.t);
    glMultiTexCoord2f(GL_TEXTURE1, dv1.s, dv1.t);
    glVertex3f(dv1.x * mRadius, dv1.y * mRadius, dv1.z * mRadius);

    for (int a = 0; a <= AzimuthSteps; a++) {
        const float azimuth = glm::radians(
            static_cast<float>(a * 360) / static_cast<float>(AzimuthSteps)
        );
            
        dv0.x =  cos(elevation0) * sin(azimuth);
        dv0.z = -cos(elevation0) * cos(azimuth);

        dv0.s = (static_cast<float>(ElevationSteps - e) /
                 static_cast<float>(ElevationSteps)) * sin(azimuth);
        dv0.t = (static_cast<float>(ElevationSteps - e) /
                 static_cast<float>(ElevationSteps)) * -cos(azimuth);
        dv0.s = (dv0.s * 0.5f) + 0.5f;
        dv0.t = (dv0.t * 0.5f) + 0.5f;
            
        glMultiTexCoord2f(GL_TEXTURE0, dv0.s, dv0.t);
        glMultiTexCoord2f(GL_TEXTURE1, dv0.s, dv0.t);
        glVertex3f(dv0.x * mRadius, dv0.y * mRadius, dv0.z * mRadius);

        mRingVertices.push_back(dv0);
    }

    mVertices.push_back(mRingVertices);
    pole = dv1;

    glEnd();
    glEndList();
}

void Dome::drawLatitudeLines(float latitude, float minLongitude, float maxLongitude,
                             int segments)
{
    glLineWidth(2);
    const float dh = (maxLongitude - minLongitude) / static_cast<float>(segments);
    
    glColor3f(1.f, 0.1f, 0.1f);
    glBegin(GL_LINE_STRIP);
    const float y = mRadius * sin(glm::radians(latitude));
    for (int a = 0; a <= segments; a++) {
        const float azimuth = glm::radians(minLongitude + static_cast<float>(a) * dh);
        const float x = mRadius * cos(glm::radians(latitude)) * sin(azimuth);
        const float z = -mRadius * cos(glm::radians(latitude)) * cos(azimuth);
        
        glVertex3f(x, y, z);
    }
    glEnd();
}

void Dome::drawLongitudeLines(float longitude, float minLatitude, float maxLatitude,
                              int segments)
{
    glLineWidth(2);
    const float dv = (maxLatitude - minLatitude) / static_cast<float>(segments);

    glColor3f(1.0f, 0.1f, 0.1f);

    glBegin(GL_LINE_STRIP);
    for (int e = 0; e <= segments; e++) {
        const float elevation = glm::radians(minLatitude + static_cast<float>(e) * dv);
        const float x = mRadius * cos(elevation) * sin(glm::radians(longitude));
        const float y = mRadius * sin(elevation);
        const float z = -mRadius * cos(elevation) * cos(glm::radians(longitude));
            
        glVertex3f(x, y, z);
    }
    glEnd();
}

void Dome::drawVerticalFrustumLine(glm::vec3 v1, glm::vec3 v2, int segments) {
    glLineWidth(3);
    glColor3f(0.1f, 0.1f, 1.f);
    glBegin(GL_LINE_STRIP);
    for (int e = 0; e <= segments; e++) {
        const float a = static_cast<float>(segments - e) / static_cast<float>(segments);
        glm::vec3 ray = glm::normalize((v1 * a) + (v2 * (1.f - a)));
        ray = glm::normalize(ray) * mRadius;
        glVertex3fv(glm::value_ptr(ray));
    }
    glEnd();
   
}

float Dome::getRadius() const {
    return mRadius;
}

void Dome::setTiltOffset(float diff) {
    mTiltOffset = diff;
}

void Dome::drawHorizontalFrustumLine(float verAngle, float minHorAngle, float maxHorAngle,
                                     int segments)
{
    glLineWidth(2);
    verAngle = glm::radians(verAngle);

    const float dh = (maxHorAngle - minHorAngle) / static_cast<float>(segments);
    
    glColor3f(0.1f, 0.1f, 1.f);
    glBegin(GL_LINE_STRIP);
    for (int a = 0; a <= segments; a++) {
        const float azimuth = glm::radians(minHorAngle + static_cast<float>(a) * dh);

        const float x = mRadius * sin(azimuth);
        const float y = mRadius * cos(azimuth) * sin(verAngle);
        const float z = -mRadius * cos(azimuth) * cos(verAngle);
        
        glVertex3f(x, y, z);
    }
    glEnd();
}
