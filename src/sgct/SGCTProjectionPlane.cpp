/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTProjectionPlane.h>

#include <sgct/MessageHandler.h>

namespace sgct_core{

sgct_core::SGCTProjectionPlane::SGCTProjectionPlane() {
    reset();
}

void SGCTProjectionPlane::configure(tinyxml2::XMLElement* element,
                                    glm::vec3* initializedCornerPoints)
{
    using namespace tinyxml2;

    const char* val;
    size_t i = 0;

    tinyxml2::XMLElement* subElement = element->FirstChildElement();
    while (subElement != nullptr) {
        val = subElement->Value();

        if (strcmp("Pos", val) == 0) {
            glm::vec3 tmpVec;
            float fTmp[3];

            if (subElement->QueryFloatAttribute("x", &fTmp[0]) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("y", &fTmp[1]) == XML_NO_ERROR &&
                subElement->QueryFloatAttribute("z", &fTmp[2]) == XML_NO_ERROR)
            {
                tmpVec.x = fTmp[0];
                tmpVec.y = fTmp[1];
                tmpVec.z = fTmp[2];

                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "SGCTProjectionPlane: Adding plane coordinates %f %f %f for corner %d\n",
                    tmpVec.x, tmpVec.y, tmpVec.z, i % 3
                );

                setCoordinate(i % 3, tmpVec);
                //Write this to initializedCornerPoints so caller knows initial corner values
                initializedCornerPoints[i].x = tmpVec.x;
                initializedCornerPoints[i].y = tmpVec.y;
                initializedCornerPoints[i].z = tmpVec.z;
                i++;
            }
            else {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "SGCTProjectionPlane: Failed to parse coordinates from XML!\n"
                );
            }
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }
}

void SGCTProjectionPlane::reset() {
    mProjectionPlaneCoords[LowerLeft] = glm::vec3(-1.f, -1.f, -2.f);
    mProjectionPlaneCoords[UpperLeft] = glm::vec3(-1.f, 1.f, -2.f);
    mProjectionPlaneCoords[UpperRight] = glm::vec3(1.f, 1.f, -2.f);
}

void SGCTProjectionPlane::offset(const glm::vec3& p) {
    mProjectionPlaneCoords[LowerLeft] += p;
    mProjectionPlaneCoords[UpperLeft] += p;
    mProjectionPlaneCoords[UpperRight] += p;
}

void SGCTProjectionPlane::setCoordinate(ProjectionPlaneCorner corner,
                                        glm::vec3 coordinate)
{
    mProjectionPlaneCoords[corner] = std::move(coordinate);
}

void SGCTProjectionPlane::setCoordinate(size_t corner, glm::vec3 coordinate) {
    mProjectionPlaneCoords[corner] = std::move(coordinate);
}

/*!
\returns coordinate pointer for the selected projection plane corner
*/
const glm::vec3* SGCTProjectionPlane::getCoordinatePtr(ProjectionPlaneCorner corner) const
{
    return &mProjectionPlaneCoords[corner];
}

/*!
\returns coordinate for selected the projection plane corner
*/
glm::vec3 SGCTProjectionPlane::getCoordinate(ProjectionPlaneCorner corner) const {
    return mProjectionPlaneCoords[corner];
}

} // namespace sgct_core
