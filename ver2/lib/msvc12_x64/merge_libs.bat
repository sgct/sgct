DEL sgct.lib
DEL sgctd.lib
call "%VS120COMNTOOLS%"\vsvars32.bat
lib /LTCG /OUT:sgct.lib sgct_light.lib deps\freetype.lib deps\glew.lib deps\glfw3.lib deps\libpng15_static.lib deps\tinythreadpp.lib deps\tinyxml2.lib deps\vrpn.lib deps\zlibstatic.lib
lib /LTCG /OUT:sgctd.lib sgct_lightd.lib deps\freetyped.lib deps\glewd.lib deps\glfw3d.lib deps\libpng15_staticd.lib deps\tinythreadppd.lib deps\tinyxml2d.lib deps\vrpnd.lib deps\zlibstaticd.lib
pause