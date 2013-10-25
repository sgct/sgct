#!/bin/bash
# merge libs
echo "Merging release libs... "
rm libsgct.a
ar x deps/libGLEW.a
ar x deps/libglfw3.a
ar x deps/libz.a
ar x deps/libpng15.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.o
rm *.o
echo "Merging debug libs... "
rm libsgctd.a
ar x deps/libGLEW.a
ar x deps/libglfw3.a
ar x deps/libz.a
ar x deps/libpng15.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2_d.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_lightd.a
ar r libsgctd.a *.o
rm *.o
echo "Done."
