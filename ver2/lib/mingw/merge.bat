DEL libsgct.a
ar x deps/libGLEW.a
ar x deps/libglfw3.a
ar x deps/libzlibstatic.a
ar x deps/libpng15.a
ar x deps/freetype.a
ar x deps/libtinyxml2.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.o *.obj
DEL *.o *.obj
pause