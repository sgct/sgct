/*
 * Mpcdi.h
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

#include <tinyxml2.h>
#include <unzip.h>
#include <zip.h>

namespace sgct { class Window; }

namespace sgct_core {

class Node;

class Mpcdi {
public:
    bool parseConfiguration(const std::string& filenameMpcdi, Node& node,
        sgct::Window& window);

private:
    struct MpcdiFoundItems {
        bool haveDisplayElem = false;
        bool haveBufferElem = false;
        glm::ivec2 resolution = glm::ivec2(-1);
    };

    struct MpcdiWarp {
        std::string id;
        std::string pathWarpFile;
        bool haveFoundPath = false;
        bool haveFoundInterpolation = false;
    };

    bool readAndParseString(Node& node, sgct::Window& win);
    bool readAndParseMpcdi(tinyxml2::XMLDocument& xmlDoc, Node& node,
        sgct::Window& win);
    bool readAndParseDisplay(tinyxml2::XMLElement* element, Node& node,
        sgct::Window& win, MpcdiFoundItems& parsedItems);
    bool readAndParseFiles(tinyxml2::XMLElement* element, sgct::Window& win);
    bool readAndParseBuffer(tinyxml2::XMLElement* element, sgct::Window& win,
        MpcdiFoundItems& parsedItems);
    bool readAndParseRegion(tinyxml2::XMLElement* element, sgct::Window& win,
        MpcdiFoundItems& parsedItems);
    bool readAndParseGeoWarpFile(tinyxml2::XMLElement* element,
        sgct::Window& win, std::string filesetRegionId);

    struct SubFile {
        bool isFound = false;
        std::string fileName;
        std::vector<char> buffer;
    };

    bool processSubFile(SubFile& sf, const std::string& suffix,
        const std::string& filename, unzFile zipfile, const unz_file_info& fileInfo);

    SubFile mXmlFileContents;
    SubFile mPfmFileContents;

    std::vector<std::string> mBufferRegions;
    std::vector<std::unique_ptr<MpcdiWarp>> mWarp;
};

} //namespace sgct_core

#endif // __SGCT__MPCDI__H__
