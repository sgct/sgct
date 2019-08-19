/*
 * SGCTMpcdi.h
 *
 *  Created on: Jul 3, 2017
 *      Author: Gene Payne
 */

#ifndef __SGCT__MPCDI__H__
#define __SGCT__MPCDI__H__

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "external/unzip.h"
#include "external/zip.h"
#ifndef SGCT_DONT_USE_EXTERNAL
    #include <external/tinyxml2.h>
#else
    #include <tinyxml2.h>
#endif

namespace sgct { class SGCTWindow; }

namespace sgct_core {

class SGCTNode;

class SGCTMpcdi {
public:
    explicit SGCTMpcdi(std::string parentErrorMessage);
    ~SGCTMpcdi();

    bool parseConfiguration(const std::string& filenameMpcdi, SGCTNode& node,
        sgct::SGCTWindow& window);

private:


    struct MpcdiFoundItems {
        bool haveDisplayElem = false;
        bool haveBufferElem = false;
        int resolutionX = -1;
        int resolutionY = -1;
    };

    struct MpcdiRegion {
        std::string id;
    };

    struct MpcdiWarp {
        std::string id;
        std::string pathWarpFile;
        bool haveFoundPath = false;
        bool haveFoundInterpolation = false;
    };

    bool readAndParseXMLString(SGCTNode& tmpNode, sgct::SGCTWindow& tmpWin);
    bool readAndParseXML_mpcdi(tinyxml2::XMLDocument& xmlDoc, SGCTNode& tmpNode,
        sgct::SGCTWindow& tmpWin);
    bool readAndParseXML_display(tinyxml2::XMLElement* element[], const char* val[],
        SGCTNode& tmpNode, sgct::SGCTWindow& tmpWin, MpcdiFoundItems& parsedItems);
    bool readAndParseXML_files(tinyxml2::XMLElement* element[], const char* val[],
        sgct::SGCTWindow& tmpWin);
    bool readAndParseXML_buffer(tinyxml2::XMLElement* element[], const char* val[],
        sgct::SGCTWindow& tmpWin, MpcdiFoundItems& parsedItems);
    bool readAndParseXML_region(tinyxml2::XMLElement* element[], const char* val[],
        sgct::SGCTWindow& tmpWin, MpcdiFoundItems& parsedItems);
    bool readAndParseXML_geoWarpFile(tinyxml2::XMLElement* element[],
        const char* val[], sgct::SGCTWindow& tmpWin,
        std::string filesetRegionId);
    bool processSubFiles(std::string filename, unzFile* zipfile,
        unz_file_info& file_info);


    //struct MpcdiSubFiles {
    //    enum MpcdiSubFileTypes {
    //        MpcdiXml = 0,
    //        MpcdiPfm,
    //        Mpcdi_nRequiredFiles //Leave at end
    //    };
    //    bool hasFound[Mpcdi_nRequiredFiles];
    //    std::string extension[Mpcdi_nRequiredFiles];
    //    std::string filename[Mpcdi_nRequiredFiles];
    //    int size[Mpcdi_nRequiredFiles];
    //    char* buffer[Mpcdi_nRequiredFiles];

    //    MpcdiSubFiles();
    //    ~MpcdiSubFiles();
    //};

    struct SubFile {
        bool hasFound = false;
        std::string extension;
        std::string fileName;
        int size;
        std::vector<char> buffer;
    };
    SubFile mXmlFileContents;
    SubFile mPfmFileContents;

    std::vector<MpcdiRegion*> mBufferRegions;
    std::vector<std::unique_ptr<MpcdiWarp>> mWarp;
    std::string mErrorMsg;
};

} //namespace sgct_core

#endif // __SGCT__MPCDI__H__
