include(CMakeFindDependencyMacro)

find_dependency(fmt)
find_dependency(unofficial-tracy)
find_dependency(glad)
find_dependency(ZLIB)
find_dependency(PNG)
find_dependency(minizip)
find_dependency(tinyxml2)
find_dependency(freetype)
find_dependency(glm)

include("${CMAKE_CURRENT_LIST_DIR}/sgctTargets.cmake")
