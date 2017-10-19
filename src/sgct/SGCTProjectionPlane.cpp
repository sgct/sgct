/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTProjectionPlane.h>
#include <sgct/MessageHandler.h>

sgct_core::SGCTProjectionPlane::SGCTProjectionPlane()
{
    reset();
}

void sgct_core::SGCTProjectionPlane::configure(tinyxml2::XMLElement * element,
                                               glm::vec3* initializedCornerPoints)
{
    const char * val;
    std::size_t i = 0;
    
    tinyxml2::XMLElement * subElement = element->FirstChildElement();
    while (subElement != NULL)
    {
        val = subElement->Value();

        if (strcmp("Pos", val) == 0)
        {
            glm::vec3 tmpVec;
            float fTmp[3];
            
            if (subElement->QueryFloatAttribute("x", &fTmp[0]) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("y", &fTmp[1]) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("z", &fTmp[2]) == tinyxml2::XML_NO_ERROR)
            {
                tmpVec.x = fTmp[0];
                tmpVec.y = fTmp[1];
                tmpVec.z = fTmp[2];

                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "SGCTProjectionPlane: Adding plane coordinates %f %f %f for corner %d\n",
                    tmpVec.x, tmpVec.y, tmpVec.z, i % 3);

                setCoordinate(i % 3, tmpVec);
                //Write this to initializedCornerPoints so caller knows initial corner values
                initializedCornerPoints[i].x = tmpVec.x;
                initializedCornerPoints[i].y = tmpVec.y;
                initializedCornerPoints[i].z = tmpVec.z;
                i++;
            }
            else
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTProjectionPlane: Failed to parse coordinates from XML!\n");
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }
}

void sgct_core::SGCTProjectionPlane::reset()
{
    mProjectionPlaneCoords[LowerLeft] = glm::vec3(-1.0f, -1.0f, -2.0f);
    mProjectionPlaneCoords[UpperLeft] = glm::vec3(-1.0f, 1.0f, -2.0f);
    mProjectionPlaneCoords[UpperRight] = glm::vec3(1.0f, 1.0f, -2.0f);
}

void sgct_core::SGCTProjectionPlane::offset(glm::vec3 p)
{
    mProjectionPlaneCoords[LowerLeft] += p;
    mProjectionPlaneCoords[UpperLeft] += p;
    mProjectionPlaneCoords[UpperRight] += p;
}

void sgct_core::SGCTProjectionPlane::setCoordinate(ProjectionPlaneCorner corner, glm::vec3 coordinate)
{
    mProjectionPlaneCoords[corner] = coordinate;
}

void sgct_core::SGCTProjectionPlane::setCoordinate(std::size_t corner, glm::vec3 coordinate)
{
    mProjectionPlaneCoords[corner] = coordinate;
}

/*!
\returns coordinate pointer for the selected projection plane corner
*/
const glm::vec3 * sgct_core::SGCTProjectionPlane::getCoordinatePtr(ProjectionPlaneCorner corner) const
{
    return &mProjectionPlaneCoords[corner];
}

/*!
\returns coordinate for selected the projection plane corner
*/
glm::vec3 sgct_core::SGCTProjectionPlane::getCoordinate(ProjectionPlaneCorner corner) const
{
    return mProjectionPlaneCoords[corner];
}
