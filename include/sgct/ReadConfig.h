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

#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif

namespace sgct_core {

class ReadConfig {
public:
    explicit ReadConfig(std::string filename);

    bool isValid() const;
    static glm::quat parseOrientationNode(tinyxml2::XMLElement* element);
    static glm::quat parseMpcdiOrientationNode(float yaw, float pitch, float roll);

private:
    bool replaceEnvVars(const std::string& filename);
    bool readAndParseXMLFile();
    bool readAndParseXMLString();
    bool readAndParseXML(tinyxml2::XMLDocument& xmlDoc);
    sgct::SGCTWindow::StereoMode getStereoType(std::string type);
    sgct::SGCTWindow::ColorBitDepth getBufferColorBitDepth(std::string type);

    bool valid;
    std::string xmlFileName;
    std::string mErrorMsg;
};

} // namespace sgct_config

#endif // __SGCT__READ_CONFIG__H__
