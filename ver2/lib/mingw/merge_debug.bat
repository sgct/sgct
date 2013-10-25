DEL libsgctd.a
ar x deps/libGLEW.a
ar x deps/libglfw3.a
ar x deps/libzlibstatic.a
ar x deps/libpng15.a
ar x deps/freetype.a
ar x deps/libtinyxml2_d.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_light_d.a
ar r libsgctd.a *.o *.obj
DEL *.o *.obj
pause