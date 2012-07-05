DEL libsgct32_d.a
ar x deps/libglew32.a
ar x deps/libglfw.a
ar x deps/libz.a
ar x deps/libpng.a
ar x deps/freetype.a
ar x deps/libtinyxml2_d.a
ar x deps/libvrpn.a
ar x libsgct32_light_d.a
ar r libsgct32_d.a *.o *.obj
DEL *.o *.obj
pause