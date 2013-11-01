DEL libsgctd.a
ar x deps/libglewd.a
ar x deps/libglfw3d.a
ar x deps/libzlibstaticd.a
ar x deps/libpng15d.a
ar x deps/freetyped.a
ar x deps/libtinyxml2d.a
ar x deps/libtinythreadppd.a
ar x deps/libvrpnd.a
ar x libsgct_lightd.a
ar r libsgctd.a *.o *.obj
DEL *.o *.obj
pause