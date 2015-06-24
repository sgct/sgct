/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_CPP_ELEVEN
#define _SGCT_CPP_ELEVEN

/*
The macros below set the propper c++ includes and namespaces
*/
#if (defined(__cplusplus) && __cplusplus >= 201103L) || _MSC_VER >= 1600 //c++11 compiler or visual studio later than 2010
    #define __USE_CPP11__ 1
    #define __USE_CPP0X__ 0
    #define __LOAD_CPP11_FUN__
    #define SGCT_NULL_PTR nullptr
    //#pragma message("SGCT will use c++11")

#elif (defined(__cplusplus) && __cplusplus > 199711L) || _MSC_VER >= 1400 //c++0x compiler or visual studio 2008 - 2010
    #define __USE_CPP11__ 0
    #define __USE_CPP0X__ 1
    #define __LOAD_CPP11_FUN__
    #define SGCT_NULL_PTR NULL
    //#pragma message "SGCT will use c++0x"

#else //older compiler
    #define __USE_CPP11__ 0
    #define __USE_CPP0X__ 0
    #define SGCT_NULL_PTR NULL
    #pragma message("Warning: C++0x/11 not supported by compiler!")
#endif

#if __USE_CPP11__
	#if defined(__APPLE__) && defined(__GLIBCXX__)
        #pragma message("libstdc++ doesn't fully support c++11 in OS X, SGCT will revert to c++0x")
        #include <tr1/functional>
        #include <tr1/unordered_map>
        #undef SGCT_NULL_PTR
        #define SGCT_NULL_PTR NULL
        namespace sgct_cppxeleven = std::tr1;
    #else
        #include <functional>
        #include <unordered_map>
        namespace sgct_cppxeleven = std;
    #endif

#elif __USE_CPP0X__
    #include <tr1/functional>
    #include <tr1/unordered_map>
    namespace sgct_cppxeleven = std::tr1;

#endif

#endif
