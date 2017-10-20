DEL libsgct.a
ar x deps/libglew.a
ar x deps/libglfw3.a
ar x deps/libzlibstatic.a
ar x deps/libminiziplibstatic.a
ar x deps/libpng16.a
ar x deps/libturbojpeg.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.obj
DEL *.o *.obj
