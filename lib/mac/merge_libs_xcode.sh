#!/bin/bash
# merge libs
if [ -f libsgct_light.a ] && [ -f libsgct_lightd.a ];
then
	echo "Merging release libs... "
	if [ -f libsgct.a ];
	then
		rm libsgct.a
	fi
	libtool -static -o libsgct.a deps/libglew.a deps/libglfw3.a deps/libz.a deps/libpng15.a deps/libfreetype.a deps/libtinyxml2.a deps/libtinythreadpp.a deps/libvrpn.a libsgct_light.a
	echo "Merging debug libs... "
	if [ -f libsgctd.a ];
	then
		rm libsgctd.a
	fi
	libtool -static -o libsgctd.a deps/libglewd.a deps/libglfw3d.a deps/libzd.a deps/libpng15d.a deps/libfreetyped.a deps/libtinyxml2d.a deps/libtinythreadppd.a deps/libvrpnd.a libsgct_lightd.a
	echo "Done."
elif [ -f libsgct_light.a ];
then
	echo "Merging release libs... "
	if [ -f libsgct.a ];
	then
		rm libsgct.a
	fi
	libtool -static -o libsgct.a deps/libglew.a deps/libglfw3.a deps/libz.a deps/libpng15.a deps/libfreetype.a deps/libtinyxml2.a deps/libtinythreadpp.a deps/libvrpn.a libsgct_light.a
	echo "Done."
elif [ -f libsgct_lightd.a ];
then
	echo "Merging debug libs... "
	if [ -f libsgctd.a ];
	then
		rm libsgctd.a
	fi
	libtool -static -o libsgctd.a deps/libglewd.a deps/libglfw3d.a deps/libzd.a deps/libpng15d.a deps/libfreetyped.a deps/libtinyxml2d.a deps/libtinythreadppd.a deps/libvrpnd.a libsgct_lightd.a
	echo "Done."
fi
