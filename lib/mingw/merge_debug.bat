DEL libsgctd.a
ar x deps/libglewd.a
ar x deps/libglfw3d.a
ar x deps/libzlibstaticd.a
ar x deps/libminiziplibstaticd.a
ar x deps/libpng16d.a
ar x deps/libturbojpegd.a
ar x deps/libfreetyped.a
ar x deps/libtinyxml2d.a
ar x deps/libvrpnd.a
ar x libsgct_lightd.a
ar r libsgctd.a *.obj
DEL *.o *.obj
pause
