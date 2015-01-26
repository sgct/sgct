#!/bin/bash
# merge libs
if [ -f libsgct_light.a ] && [ -f libsgct_lightd.a ];
then
	echo "Merging release libs... "
	if [ -f libsgct_cpp11.a ];
	then
		rm libsgct_cpp11.a
	fi
	libtool -static -o libsgct_cpp11.a deps/libglew.a deps/libglfw3.a deps/libz.a deps/libpng15.a deps/libturbojpeg.a deps/libfreetype.a deps/libtinyxml2.a deps/libtinythreadpp.a deps/libvrpn.a libsgct_light.a
	echo "Merging debug libs... "
	if [ -f libsgct_cpp11d.a ];
	then
		rm libsgct_cpp11d.a
	fi
	libtool -static -o libsgct_cpp11d.a deps/libglewd.a deps/libglfw3d.a deps/libzd.a deps/libpng15d.a deps/libturbojpegd.a deps/libfreetyped.a deps/libtinyxml2d.a deps/libtinythreadppd.a deps/libvrpnd.a libsgct_lightd.a
	echo "Done."
elif [ -f libsgct_light.a ];
then
	echo "Merging release libs... "
	if [ -f libsgct_cpp11.a ];
	then
		rm libsgct_cpp11.a
	fi
	libtool -static -o libsgct_cpp11.a deps/libglew.a deps/libglfw3.a deps/libz.a deps/libpng15.a deps/libturbojpeg.a deps/libfreetype.a deps/libtinyxml2.a deps/libtinythreadpp.a deps/libvrpn.a libsgct_light.a
	echo "Done."
elif [ -f libsgct_lightd.a ];
then
	echo "Merging debug libs... "
	if [ -f libsgct_cpp11d.a ];
	then
		rm libsgct_cpp11d.a
	fi
	libtool -static -o libsgct_cpp11d.a deps/libglewd.a deps/libglfw3d.a deps/libzd.a deps/libpng15d.a deps/libturbojpegd.a deps/libfreetyped.a deps/libtinyxml2d.a deps/libtinythreadppd.a deps/libvrpnd.a libsgct_lightd.a
	echo "Done."
fi
