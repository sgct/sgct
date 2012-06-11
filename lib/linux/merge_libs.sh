#!/bin/bash
# merge libs
echo "Merging libs... "
ar x deps/libGLEW.a
ar x deps/libglfw.a
ar x deps/libzlib.a
ar x deps/libpng15.a
ar x deps/libfreetype.a
ar x deps/libtinyxml.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.o
rm *.o
echo "Done."
