ar x ../../deps/mingw/lib/libglew32.a
ar x ../../deps/mingw/lib/libglfw.a
ar x ../../deps/mingw/lib/libz.a
ar x ../../deps/mingw/lib/libpng.a
ar x ../../deps/mingw/lib/freetype.a
ar x ../../deps/mingw/lib/libtinyxml.a
ar x libsgct32_light.a
ar r libsgct32.a *.o
DEL *.o
pause