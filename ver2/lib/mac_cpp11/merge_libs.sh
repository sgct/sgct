#!/bin/bash
# merge libs
if [ -f libsgct_light.a ] && [ -f libsgct_lightd.a ];
then
	echo "Merging release libs... "
	if [ -f libsgct_cpp11.a ];
	then
		rm libsgct_cpp11.a
	fi
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
	if [ -f libsgct_cpp11d.a ];
	then
		rm libsgct_cpp11d.a
	fi
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
elif [ -f libsgct_light.a ];
then
	echo "Merging release libs... "
	if [ -f libsgct_cpp11.a ];
	then
		rm libsgct_cpp11.a
	fi
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
	echo "Done."
elif [ -f libsgct_lightd.a ];
then
	echo "Merging debug libs... "
	if [ -f libsgct_cpp11d.a ];
	then
		rm libsgct_cpp11d.a
	fi
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
fi