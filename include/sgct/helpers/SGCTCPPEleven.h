/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_CPP_ELEVEN
#define _SGCT_CPP_ELEVEN

/*
The macros below set the propper c++11 includes and namespaces
*/
#if defined(__cplusplus) && __cplusplus > 199711L //c++11 or c++0x compiler
#define __USE_CPP11__ 1
#else //older compiler
#define __USE_CPP11__ 0
#endif

#if __USE_CPP11__ || _MSC_VER >= 1600 //c++11 or visual studio >= 2010
#include <functional>
#if defined(__APPLE__) && defined(_GLIBCXX_FUNCTIONAL) //incorrect header loaded
#include <tr1/functional>
namespace sgct_cppxeleven = std::tr1;
//#pragma message "SGCTNetwork will use std:tr1::functional"
#else
namespace sgct_cppxeleven = std;
//#pragma message "SGCTNetwork will use std::functional"
#endif
#define __LOAD_CPP11_FUN__
#elif _MSC_VER >= 1400 //visual studio 2005 or later
#include <functional>
namespace sgct_cppxeleven = std::tr1;
//#pragma message "SGCTNetwork will use std::tr1::functional"
#define __LOAD_CPP11_FUN__
#elif __USE_CPP11__ || defined(__LINUX__)
#include <tr1/functional>
namespace sgct_cppxeleven = std::tr1;
//#pragma message "SGCTNetwork will use std:tr1::functional"
#define __LOAD_CPP11_FUN__
#endif

#endif
