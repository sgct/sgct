ar x deps/libglew32.a
ar x deps/libglfw.a
ar x deps/libz.a
ar x deps/libpng.a
ar x deps/freetype.a
ar x deps/libtinyxml.a
ar x deps/libvrpn.a
ar x libsgct32_light.a
ar r libsgct32.a *.o *.obj
DEL *.o *.obj
pause