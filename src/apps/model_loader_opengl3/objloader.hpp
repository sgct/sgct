#ifndef __OBJLOADER_H__
#define __OBJLOADER_H__

#include <glm/glm.hpp>
#include <string>
#include <vector>

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& vertices, 
	std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals
);

#endif // __OBJLOADER_H__
