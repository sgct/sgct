/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTProjectionPlane.h>

#include <sgct/MessageHandler.h>

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/tinyxml2.h"
#else
#include <tinyxml2.h>
#endif

namespace sgct_core{

sgct_core::SGCTProjectionPlane::SGCTProjectionPlane() {
    reset();
}

void SGCTProjectionPlane::configure(tinyxml2::XMLElement* element,
                                    glm::vec3& initializedLowerLeftCorner,
                                    glm::vec3& initializedUpperLeftCorner,
                                    glm::vec3& initializedUpperRightCorner)
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

                switch (i % 3) {
                    case 0:
                        setCoordinateLowerLeft(tmpVec);
                        initializedLowerLeftCorner = tmpVec;
                        break;
                    case 1:
                        setCoordinateUpperLeft(tmpVec);
                        initializedUpperLeftCorner = tmpVec;
                        break;
                    case 2:
                        setCoordinateUpperRight(tmpVec);
                        initializedUpperRightCorner = tmpVec;
                        break;
                }

                i++;
            }
            else {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "SGCTProjectionPlane: Failed to parse coordinates from XML!\n"
                );
            }
        }

        subElement = subElement->NextSiblingElement();
    }
}

void SGCTProjectionPlane::reset() {
    mProjectionPlaneCoords.lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    mProjectionPlaneCoords.upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    mProjectionPlaneCoords.upperRight = glm::vec3(1.f, 1.f, -2.f);
}

void SGCTProjectionPlane::offset(const glm::vec3& p) {
    mProjectionPlaneCoords.lowerLeft += p;
    mProjectionPlaneCoords.upperLeft += p;
    mProjectionPlaneCoords.upperRight += p;
}

void SGCTProjectionPlane::setCoordinateLowerLeft(glm::vec3 coordinate) {
    mProjectionPlaneCoords.lowerLeft = std::move(coordinate);
}

void SGCTProjectionPlane::setCoordinateUpperLeft(glm::vec3 coordinate) {
    mProjectionPlaneCoords.upperLeft = std::move(coordinate);

}

void SGCTProjectionPlane::setCoordinateUpperRight(glm::vec3 coordinate) {
    mProjectionPlaneCoords.upperRight = std::move(coordinate);

}

glm::vec3 SGCTProjectionPlane::getCoordinateLowerLeft() const {
    return mProjectionPlaneCoords.lowerLeft;
}

glm::vec3 SGCTProjectionPlane::getCoordinateUpperLeft() const {
    return mProjectionPlaneCoords.upperLeft;
}

glm::vec3 SGCTProjectionPlane::getCoordinateUpperRight() const {
    return mProjectionPlaneCoords.upperRight;
}

} // namespace sgct_core
