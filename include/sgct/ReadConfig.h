/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.

 For conditions of distribution and use, see copyright notice in sgct.h 
 *************************************************************************/

#ifndef __SGCT__READ_CONFIG__H__
#define __SGCT__READ_CONFIG__H__

#include <sgct/window.h>
#include <glm/glm.hpp>
#include <string>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
} // namespace tinyxml2

namespace sgct_core::readconfig {

glm::quat parseMpcdiOrientationNode(float yaw, float pitch, float roll);
glm::quat parseOrientationNode(tinyxml2::XMLElement* element);

void readConfig(const std::string& filename);

} // namespace sgct_config

#endif // __SGCT__READ_CONFIG__H__
