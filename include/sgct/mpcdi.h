/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MPCDI__H__
#define __SGCT__MPCDI__H__

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include <tinyxml2.h>
#include <unzip.h>
#include <zip.h>

namespace sgct { class Window; }

namespace sgct::core {

class Node;

class Mpcdi {
public:
    bool parseConfiguration(const std::string& filenameMpcdi, Window& window);

private:
    struct MpcdiFoundItems {
        bool hasDisplayElem = false;
        bool hasBufferElem = false;
        glm::ivec2 resolution = glm::ivec2(-1);
    };

    struct MpcdiWarp {
        std::string id;
        std::string pathWarpFile;
        bool hasFoundPath = false;
        bool hasFoundInterpolation = false;
    };

    bool readAndParseString(Window& win);
    bool readAndParseMpcdi(tinyxml2::XMLDocument& xmlDoc, Window& win);
    bool readAndParseDisplay(tinyxml2::XMLElement* element, Window& win,
        MpcdiFoundItems& parsedItems);
    bool readAndParseFiles(tinyxml2::XMLElement* element, Window& win);
    bool readAndParseBuffer(tinyxml2::XMLElement* element, Window& win,
        MpcdiFoundItems& parsedItems);
    bool readAndParseRegion(tinyxml2::XMLElement* element, Window& win,
        MpcdiFoundItems& parsedItems);
    bool readAndParseGeoWarpFile(tinyxml2::XMLElement* element, Window& win,
        std::string filesetRegionId);

    struct SubFile {
        bool isFound = false;
        std::string fileName;
        std::vector<char> buffer;
    };

    bool processSubFile(SubFile& sf, const std::string& suffix,
        const std::string& filename, unzFile zipfile, const unz_file_info& fileInfo);

    SubFile _xmlFileContents;
    SubFile _pfmFileContents;

    std::vector<std::string> _bufferRegions;
    std::vector<std::unique_ptr<MpcdiWarp>> _warp;
};

} //namespace sgct::core

#endif // __SGCT__MPCDI__H__
