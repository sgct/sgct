if exist libsgct_light.a (
	if exist libsgct_lightd.a (
		goto CREATE_ALL
	)
)

if exist libsgct_light.a goto CREATE_RELEASE
if exist libsgct_lightd.a goto CREATE_DEBUG

:CREATE_ALL
DEL libsgct.a
ar x deps/libglew.a
ar x deps/libglfw3.a
ar x deps/libzlibstatic.a
ar x deps/libpng15.a
ar x deps/libturbojpeg.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.obj
DEL libsgctd.a
ar x deps/libglewd.a
ar x deps/libglfw3d.a
ar x deps/libzlibstaticd.a
ar x deps/libpng15d.a
ar x deps/libturbojpegd.a
ar x deps/libfreetyped.a
ar x deps/libtinyxml2d.a
ar x deps/libtinythreadppd.a
ar x deps/libvrpnd.a
ar x libsgct_lightd.a
ar r libsgctd.a *.obj
DEL *.o *.obj
goto END

:CREATE_DEBUG
DEL libsgctd.a
ar x deps/libglewd.a
ar x deps/libglfw3d.a
ar x deps/libzlibstaticd.a
ar x deps/libpng15d.a
ar x deps/libturbojpegd.a
ar x deps/libfreetyped.a
ar x deps/libtinyxml2d.a
ar x deps/libtinythreadppd.a
ar x deps/libvrpnd.a
ar x libsgct_lightd.a
ar r libsgctd.a *.obj
DEL *.o *.obj
goto END

:CREATE_RELEASE
DEL libsgct.a
ar x deps/libglew.a
ar x deps/libglfw3.a
ar x deps/libzlibstatic.a
ar x deps/libpng15.a
ar x deps/libturbojpeg.a
ar x deps/libfreetype.a
ar x deps/libtinyxml2.a
ar x deps/libtinythreadpp.a
ar x deps/libvrpn.a
ar x libsgct_light.a
ar r libsgct.a *.obj
DEL *.o *.obj
goto END

:END