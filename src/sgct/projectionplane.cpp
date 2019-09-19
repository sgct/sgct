/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/projectionplane.h>

#include <sgct/messagehandler.h>
#include <tinyxml2.h>

namespace sgct::core {

void ProjectionPlane::configure(tinyxml2::XMLElement* element,
                                glm::vec3& initializedLowerLeftCorner,
                                glm::vec3& initializedUpperLeftCorner,
                                glm::vec3& initializedUpperRightCorner)
{
    using namespace tinyxml2;
    size_t i = 0;

    tinyxml2::XMLElement* elem = element->FirstChildElement();
    while (elem) {
        std::string_view val = elem->Value();

        if (val == "Pos") {
            glm::vec3 pos;

            if (elem->QueryFloatAttribute("x", &pos[0]) == XML_NO_ERROR &&
                elem->QueryFloatAttribute("y", &pos[1]) == XML_NO_ERROR &&
                elem->QueryFloatAttribute("z", &pos[2]) == XML_NO_ERROR)
            {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ProjectionPlane: Adding plane coordinates %f %f %f for corner %d\n",
                    pos.x, pos.y, pos.z, i % 3
                );

                switch (i % 3) {
                    case 0:
                        setCoordinateLowerLeft(pos);
                        initializedLowerLeftCorner = pos;
                        break;
                    case 1:
                        setCoordinateUpperLeft(pos);
                        initializedUpperLeftCorner = pos;
                        break;
                    case 2:
                        setCoordinateUpperRight(pos);
                        initializedUpperRightCorner = pos;
                        break;
                }

                i++;
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "ProjectionPlane: Failed to parse coordinates from XML\n"
                );
            }
        }

        elem = elem->NextSiblingElement();
    }
}

void ProjectionPlane::reset() {
    mPlaneCoords.lowerLeft = glm::vec3(-1.f, -1.f, -2.f);
    mPlaneCoords.upperLeft = glm::vec3(-1.f, 1.f, -2.f);
    mPlaneCoords.upperRight = glm::vec3(1.f, 1.f, -2.f);
}

void ProjectionPlane::offset(const glm::vec3& p) {
    mPlaneCoords.lowerLeft += p;
    mPlaneCoords.upperLeft += p;
    mPlaneCoords.upperRight += p;
}

void ProjectionPlane::setCoordinateLowerLeft(glm::vec3 coordinate) {
    mPlaneCoords.lowerLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperLeft(glm::vec3 coordinate) {
    mPlaneCoords.upperLeft = std::move(coordinate);
}

void ProjectionPlane::setCoordinateUpperRight(glm::vec3 coordinate) {
    mPlaneCoords.upperRight = std::move(coordinate);
}

glm::vec3 ProjectionPlane::getCoordinateLowerLeft() const {
    return mPlaneCoords.lowerLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperLeft() const {
    return mPlaneCoords.upperLeft;
}

glm::vec3 ProjectionPlane::getCoordinateUpperRight() const {
    return mPlaneCoords.upperRight;
}

} // namespace sgct::core
