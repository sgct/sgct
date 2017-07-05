/*
 * SGCTMpcdi.h
 *
 *  Created on: Jul 3, 2017
 *      Author: openspace
 */

#ifndef _SGCT_MPCDI
#define _SGCT_MPCDI

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "SGCTWindow.h"
#include "SGCTNode.h"
#include "external/unzip.h"
#include "external/zip.h"
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

class SGCTMpcdi
{
public:
    SGCTMpcdi(std::string& parentErrorMessage);
    virtual ~SGCTMpcdi();
    bool parseConfiguration(const std::string filenameMpcdi, SGCTNode& tmpNode,
             sgct::SGCTWindow& tmpWin);

private:
    bool readAndParseXMLString(SGCTNode& tmpNode, sgct::SGCTWindow& tmpWin);
    bool readAndParseXML_mpcdi(tinyxml2::XMLDocument& xmlDoc, SGCTNode tmpNode,
             sgct::SGCTWindow& tmpWin);
    bool readAndParseXML_display(tinyxml2::XMLElement* element[], const char* val[],
             SGCTNode tmpNode, sgct::SGCTWindow& tmpWin, mpcdiFoundItems& parsedItems);
    bool readAndParseXML_files(tinyxml2::XMLElement* element[], const char* val[],
             sgct::SGCTWindow& tmpWin);
    bool readAndParseXML_buffer(tinyxml2::XMLElement* element[], const char* val[],
             sgct::SGCTWindow& tmpWin, mpcdiFoundItems& parsedItems);
    bool readAndParseXML_region(tinyxml2::XMLElement* element[], const char* val[],
             sgct::SGCTWindow& tmpWin, mpcdiFoundItems& parsedItems);
    bool readAndParseXML_geoWarpFile(tinyxml2::XMLElement* element[],
             const char* val[], sgct::SGCTWindow& tmpWin,
             std::string filesetRegionId);
    bool openZipFile(FILE* cfgFile, const std::string cfgFilePath, unzFile* zipfile);
    bool processSubFiles(std::string filename, unzFile* zipfile,
             unz_file_info& file_info);
    bool doesStringHaveSuffix(const std::string &str, const std::string &suffix);
    bool checkAttributeForExpectedValue(tinyxml2::XMLElement* elem,
             const std::string attrRequired, const std::string tagDescription,
             const std::string expectedTag);
    void unsupportedFeatureCheck(std::string tag, std::string featureName);

    mpcdiSubFiles mMpcdiSubFileContents;
    std::vector<mpcdiRegion*> mBufferRegions;
    std::vector<mpcdiWarp*> mWarp;
    std::string mErrorMsg;
};

} //namespace sgct_core

#endif //#ifdef SGCT_MPCDI
