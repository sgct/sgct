#!/bin/bash
# merge libs
echo "Merging release libs... "
rm libsgct_cpp11.a
libtool -static -o libsgct_cpp11.a deps/libglew.a deps/libglfw3.a deps/libz.a deps/libpng15.a deps/libfreetype.a deps/libtinyxml2.a deps/libtinythreadpp.a deps/libvrpn.a libsgct_light.a
echo "Merging debug libs... "
rm libsgct_cpp11d.a
libtool -static -o libsgct_cpp11d.a deps/libglewd.a deps/libglfw3d.a deps/libzd.a deps/libpng15d.a deps/libfreetyped.a deps/libtinyxml2d.a deps/libtinythreadppd.a deps/libvrpnd.a libsgct_lightd.a
echo "Done."
