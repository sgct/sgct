#!/bin/bash
# merge libs
echo "Merging libs... "
rm libsgct.a
ar x deps/libglew32.a
ar x deps/libglfw.a
ar x deps/libz.a
ar x deps/libpng.a
ar x deps/libfreetype.a
ar x deps/libtinyxml.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.o
rm *.o
echo "Done."
