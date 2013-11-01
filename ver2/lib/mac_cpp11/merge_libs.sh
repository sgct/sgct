#!/bin/bash
# merge libs
echo "Merging release libs... "
rm libsgct_cpp11.a
ar x deps/libglew.a
ar x deps/libglfw3.a
ar x deps/libz.a
ar x deps/libpng15.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct_cpp11.a *.o
rm *.o
echo "Merging debug libs... "
rm libsgct_cpp11d.a
ar x deps/libglewd.a
ar x deps/libglfw3d.a
ar x deps/libzd.a
ar x deps/libpng15d.a
ar x deps/libfreetyped.a
ar x deps/libtinyxml2d.a
ar x deps/libtinythreadppd.a
ar x deps/libvrpnd.a
ar x libsgct_lightd.a
ar r libsgct_cpp11d.a *.o
rm *.o
echo "Done."
