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
#include "SGCTNode.h"

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
    bool hasFound[mpcdi_nRequiredFiles];
    std::string extension[mpcdi_nRequiredFiles];
    std::string filename[mpcdi_nRequiredFiles];
    int size[mpcdi_nRequiredFiles];
    char* buffer[mpcdi_nRequiredFiles];

    mpcdiSubFiles() {
        for (int i = 0; i < mpcdi_nRequiredFiles; ++i) {
            hasFound[i] = false;
            buffer[i] = nullptr;
        }
		extension[mpcdiXml] = "xml";
		extension[mpcdiPfm] = "pfm";
    }

    ~mpcdiSubFiles() {
        for (int i = 0; i < mpcdi_nRequiredFiles; ++i) {
            if( buffer[i] != nullptr )
                delete buffer[i];
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

struct mpcdiFoundItems {
    bool haveDisplayElem = false;
    bool haveBufferElem = false;
    int resolutionX = -1;
    int resolutionY = -1;
};

class ReadConfig
{
public:
    ReadConfig( const std::string filename );
    ~ReadConfig();

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
    void parseMpcdiConfiguration(const std::string filenameMpcdi, SGCTNode& tmpNode,
             sgct::SGCTWindow& tmpWin);
    bool readAndParseMpcdiXMLString(SGCTNode& tmpNode, sgct::SGCTWindow& tmpWin);
    bool readAndParseMpcdiXML(tinyxml2::XMLDocument& xmlDoc, SGCTNode tmpNode,
             sgct::SGCTWindow& tmpWin);
    bool readAndParseMpcdiXML_display(tinyxml2::XMLElement* element[], const char* val[],
             SGCTNode tmpNode, sgct::SGCTWindow& tmpWin, mpcdiFoundItems& parsedItems);
    bool readAndParseMpcdiXML_files(tinyxml2::XMLElement* element[], const char* val[],
             sgct::SGCTWindow& tmpWin);
    bool readAndParseMpcdiXML_buffer(tinyxml2::XMLElement* element[], const char* val[],
             sgct::SGCTWindow& tmpWin, mpcdiFoundItems& parsedItems);
    bool readAndParseMpcdiXML_region(tinyxml2::XMLElement* element[], const char* val[],
             sgct::SGCTWindow& tmpWin, mpcdiFoundItems& parsedItems);
    bool readAndParseMpcdiXML_geoWarpFile(tinyxml2::XMLElement* element[],
             const char* val[], sgct::SGCTWindow& tmpWin,
             std::string filesetRegionId);
    bool openZipFile(FILE* cfgFile, const std::string cfgFilePath, unzFile* zipfile);
    bool processMpcdiSubFiles(std::string filename, unzFile* zipfile,
             unz_file_info& file_info);
    bool doesStringHaveSuffix(const std::string &str, const std::string &suffix);
    bool checkAttributeForExpectedValue(tinyxml2::XMLElement* elem,
             const std::string attrRequired, const std::string tagDescription,
             const std::string expectedTag);
    void unsupportedFeatureCheck(std::string tag, std::string featureName);

    bool valid;
    std::string xmlFileName;
    std::string mErrorMsg;
    mpcdiSubFiles mMpcdiSubFileContents;
    std::vector<mpcdiRegion*> mBufferRegions;
    std::vector<mpcdiWarp*> mWarp;
};

}

#endif
