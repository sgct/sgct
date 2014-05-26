OS X - Macintosh
#############################################################
Install the libs to: /usr/local/lib/ and the include directory to: /usr/local/include/
Note that on later or new systems these directories doesn't exists and mkdir must be used.

The four SGCT libraries are compiled with other settings for providing compability with different types of third party libs and frameworks.

Compatible with most binary packages such as open scene graph:
=======================================================================
libsgct.a - Release build compiled with -std=c++0x -stdlib=libstdc++
libsgctd.a - Debug build compiled with -std=c++0x -stdlib=libstdc++

Compatible with new binary packages that uses the later c++11 standard
=======================================================================
libsgct_cpp11.a - Release build compiled with -std=c++11 -stdlib=libc++
libsgct_cpp11d.a - Debug build compiled with -std=c++11 -stdlib=libc++

Linux/Unix
#############################################################
Install the libs to: /usr/local/lib/ and the include directory to: /usr/local/include/
