/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.

 For conditions of distribution and use, see copyright notice in sgct.h 
 *************************************************************************/

#ifndef __SGCT__READ_CONFIG__H__
#define __SGCT__READ_CONFIG__H__

#include <sgct/SGCTWindow.h>
#include <glm/glm.hpp>
#include <string>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
} // namespace tinyxml2

namespace sgct_core {

glm::quat parseMpcdiOrientationNode(float yaw, float pitch, float roll);
glm::quat parseOrientationNode(tinyxml2::XMLElement* element);

class ReadConfig {
public:
    explicit ReadConfig(std::string filename);

private:
    // returns an empty string if the replacement fails
    std::string replaceEnvVars(const std::string& filename);
    void readAndParseXMLFile(const std::string& filename);
    void readAndParseXMLString();
    void readAndParseXML(tinyxml2::XMLDocument& xmlDoc, const std::string& filename);
};

} // namespace sgct_config

#endif // __SGCT__READ_CONFIG__H__
