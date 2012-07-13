#!/bin/bash
# merge libs
echo "Merging release libs... "
rm libsgct.a
ar x deps/libGLEW.a
ar x deps/libglfw.a
ar x deps/libzlib.a
ar x deps/libpng15.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.o
rm *.o
echo "Merging debug libs... "
rm libsgct_d.a
ar x deps/libGLEW.a
ar x deps/libglfw.a
ar x deps/libzlib.a
ar x deps/libpng15.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2_d.a
ar x deps/libvrpn.a
ar x libsgct_light_d.a
ar r libsgct_d.a *.o
rm *.o
echo "Done."
