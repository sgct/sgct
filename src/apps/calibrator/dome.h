#ifndef _DOME_
#define _DOME_

#include <glm/glm.hpp>
#include <vector>

class Dome {
public:
    enum class PatternMode : int {
        Step = 0,
        StepInverted,
        Solid,
        Gradient,
        GradientInverted
    };

    Dome(float radius, float tilt);
    ~Dome();
    void drawGeoCorrPattern();
    void drawChannelZones();
    void drawBlendZones();
    void drawTexturedSphere();
    void drawColCorrPattern(glm::vec3* color, PatternMode mode);
    void generateDisplayList();
    float getRadius() const;
    void setTiltOffset(float diff);

private:
    struct DomeVertex {
        float x, y, z, s, t;
    };

    void drawLatitudeLines(float latitude, float minLongitude, float maxLongitude, int segments);
    void drawLongitudeLines(float longitude, float minLatitude, float maxLatitude, int segments);
    void drawVerticalFrustumLine(glm::vec3 v1, glm::vec3 v2, int segments);
    void drawHorizontalFrustumLine(float verAngle, float minHorAngle, float maxHorAngle, int segments);

    float mRadius;
    float mTilt;
    float mTiltOffset = 0.f;

    unsigned int mGeoDisplayList = 0;
    unsigned int mBlendZonesDisplayList = 0;
    unsigned int mChannelZonesDisplayList = 0;
    unsigned int mTexDisplayList = 0;

    std::vector<std::vector<DomeVertex>> mVertices;
    //std::vector<DomeVertex> mCapVertices;
};

#endif