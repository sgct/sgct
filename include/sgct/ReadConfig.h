/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_READ_CONFIG
#define _SGCT_READ_CONFIG

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "SGCTWindow.h"

#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif

namespace sgct_core //simple graphics cluster toolkit
{
struct mpcdiSubFiles {
    enum mpcdiSubFileTypes {
        mpcdiXml = 0,
        mpcdiPfm,
        mpcdi_nRequiredFiles //Leave at end
    };
    bool hasFoundFile[mpcdi_nRequiredFiles];
    string subFileExtension[mpcdi_nRequiredFiles];
    int subFileSize[mpcdi_nRequiredFiles];
    char* subFileBuffer[mpcdi_nRequiredFiles];

    mpcdiSubFiles() {
        for (int i = 0; i < mpcdi_nRequiredFiles; ++i) {
            hasFoundFile[i] = false;
            subFileBuffer[i] = nullptr;
        }
    }
};

struct mpcdiRegion {
    std::string id;
};

struct mpcdiWarp {
    std::string id;
    std::string pathWarpFile;
    bool haveFoundPath = false;
    bool haveFoundInterpolation = false;
};

class ReadConfig
{
public:
    ReadConfig( const std::string filename );

    bool isValid() { return valid; }
    static glm::quat parseOrientationNode(tinyxml2::XMLElement* element);
    static glm::quat parseMpcdiOrientationNode(const float yaw, const float pitch, const float roll);

private:
    bool replaceEnvVars( const std::string &filename );
    bool readAndParseXMLFile();
    bool readAndParseXMLString();
    bool readAndParseXML(tinyxml2::XMLDocument& xmlDoc);
    sgct::SGCTWindow::StereoMode getStereoType( std::string type );
    sgct::SGCTWindow::ColorBitDepth getBufferColorBitDepth(std::string type);
    void parseMpcdiConfiguration(const std::string filenameMpcdi, sgct::SGCTWindow& tmpWin);
    bool openZipFile(FILE* cfgFile, const std::string cfgFilePath, unzFile* zipfile);
    bool processMpcdiSubFile(std::string filename, unzFile* zipfile, unz_global_info& file_info);
    bool doesStringHaveSuffix(const std::string &str, const std::string &suffix);
    void unsupportedFeatureCheck(std::string tag, std::string featureName);

    bool valid;
    std::string xmlFileName;
    std::string mErrorMsg;
    mpcdiSubFiles mMpcdiSubFileContents;
};

}

#endif
