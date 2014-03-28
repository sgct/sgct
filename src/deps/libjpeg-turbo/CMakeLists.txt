#
# Setup
#

cmake_minimum_required(VERSION 2.8.8)
cmake_policy(SET CMP0022 OLD)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

project(libjpeg-turbo C)
set(VERSION 1.3.1)

if(MINGW OR CYGWIN)
  execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
  string(REGEX REPLACE "\n" "" BUILD "${BUILD}")
elseif(WIN32)
  execute_process(COMMAND "wmic.exe" "os" "get" "LocalDateTime" OUTPUT_VARIABLE
    BUILD)
  string(REGEX REPLACE "[^0-9]" "" BUILD "${BUILD}")
  if (BUILD STREQUAL "")
    execute_process(COMMAND "cmd.exe" "/C" "DATE" "/T" OUTPUT_VARIABLE BUILD)
    string(REGEX REPLACE ".*[ ]([0-9]*)[/.]([0-9]*)[/.]([0-9]*).*" "\\3\\2\\1" BUILD "${BUILD}")
  else()
    string(SUBSTRING "${BUILD}" 0 8 BUILD)
  endif()
else()
  message(FATAL_ERROR "Platform not supported by this build system.  Use autotools instead.")
endif()

# This does nothing except when using MinGW.  CMAKE_BUILD_TYPE has no meaning
# in Visual Studio, and it always defaults to Debug when using NMake.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# This only works if building from the command line.  There is currently no way
# to set a variable's value based on the build type when using Visual Studio.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD "${BUILD}d")
endif()

message(STATUS "VERSION = ${VERSION}, BUILD = ${BUILD}")

option(WITH_SIMD "Include SIMD extensions" TRUE)
option(WITH_ARITH_ENC "Include arithmetic encoding support" TRUE)
option(WITH_ARITH_DEC "Include arithmetic decoding support" TRUE)
option(WITH_JPEG7 "Emulate libjpeg v7 API/ABI (this makes libjpeg-turbo backward incompatible with libjpeg v6b)" FALSE)
option(WITH_JPEG8 "Emulate libjpeg v8 API/ABI (this makes libjpeg-turbo backward incompatible with libjpeg v6b)" FALSE)
option(WITH_MEM_SRCDST "Include in-memory source/destination manager functions when emulating the libjpeg v6b or v7 API/ABI" TRUE)

if(WITH_ARITH_ENC)
  set(C_ARITH_CODING_SUPPORTED 1)
  message(STATUS "Arithmetic encoding support enabled")
else()
  message(STATUS "Arithmetic encoding support disabled")
endif()

if(WITH_ARITH_DEC)
  set(D_ARITH_CODING_SUPPORTED 1)
  message(STATUS "Arithmetic decoding support enabled")
else()
  message(STATUS "Arithmetic decoding support disabled")
endif()

set(SO_AGE 0)
if(WITH_MEM_SRCDST)
  set(SO_AGE 1)
endif()

set(JPEG_LIB_VERSION 62)
set(DLL_VERSION ${JPEG_LIB_VERSION})
set(FULLVERSION ${DLL_VERSION}.${SO_AGE}.0)
if(WITH_JPEG8)
  set(JPEG_LIB_VERSION 80)
  set(DLL_VERSION 8)
  set(FULLVERSION ${DLL_VERSION}.0.2)
  message(STATUS "Emulating libjpeg v8 API/ABI")
elseif(WITH_JPEG7)
  set(JPEG_LIB_VERSION 70)
  set(DLL_VERSION 7)
  set(FULLVERSION ${DLL_VERSION}.${SO_AGE}.0)
  message(STATUS "Emulating libjpeg v7 API/ABI")
endif(WITH_JPEG8)

if(WITH_MEM_SRCDST)
  set(MEM_SRCDST_SUPPORTED 1)
  message(STATUS "In-memory source/destination managers enabled")
else()
  message(STATUS "In-memory source/destination managers disabled")
endif()

if(MSVC)
  # Use the static C library for all build types
  foreach(var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
    endif()
  endforeach()

  add_definitions(-W3 -wd4996)
endif()

# Detect whether compiler is 64-bit
if(MSVC AND CMAKE_CL_64)
  set(SIMD_X86_64 1)
  set(64BIT 1)
elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(SIMD_X86_64 1)
  set(64BIT 1)
endif()

if(64BIT)
  message(STATUS "64-bit build")
else()
  message(STATUS "32-bit build")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(MSVC)
    set(CMAKE_INSTALL_PREFIX_DEFAULT ${CMAKE_PROJECT_NAME})
  else()
    set(CMAKE_INSTALL_PREFIX_DEFAULT ${CMAKE_PROJECT_NAME}-gcc)
  endif()
  if(64BIT)
    set(CMAKE_INSTALL_PREFIX_DEFAULT ${CMAKE_INSTALL_PREFIX_DEFAULT}64)
  endif()
  set(CMAKE_INSTALL_PREFIX "c:/${CMAKE_INSTALL_PREFIX_DEFAULT}" CACHE PATH
    "Directory into which to install libjpeg-turbo (default: c:/${CMAKE_INSTALL_PREFIX_DEFAULT})"
    FORCE)
endif()

message(STATUS "Install directory = ${CMAKE_INSTALL_PREFIX}")

configure_file(win/jconfig.h.in jconfig.h)
configure_file(win/config.h.in config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Targets
#

set(JPEG_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/jcapimin.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcapistd.c
	${CMAKE_CURRENT_SOURCE_DIR}/jccoefct.c
	${CMAKE_CURRENT_SOURCE_DIR}/jccolor.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcdctmgr.c
	${CMAKE_CURRENT_SOURCE_DIR}/jchuff.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcinit.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcmainct.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcmarker.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcmaster.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcomapi.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcparam.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcphuff.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcprepct.c
	${CMAKE_CURRENT_SOURCE_DIR}/jcsample.c
	${CMAKE_CURRENT_SOURCE_DIR}/jctrans.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdapimin.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdapistd.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdatadst.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdatasrc.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdcoefct.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdcolor.c
	${CMAKE_CURRENT_SOURCE_DIR}/jddctmgr.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdhuff.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdinput.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdmainct.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdmarker.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdmaster.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdmerge.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdphuff.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdpostct.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdsample.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdtrans.c
	${CMAKE_CURRENT_SOURCE_DIR}/jerror.c
	${CMAKE_CURRENT_SOURCE_DIR}/jfdctflt.c
	${CMAKE_CURRENT_SOURCE_DIR}/jfdctfst.c
	${CMAKE_CURRENT_SOURCE_DIR}/jfdctint.c
	${CMAKE_CURRENT_SOURCE_DIR}/jidctflt.c
	${CMAKE_CURRENT_SOURCE_DIR}/jidctfst.c
	${CMAKE_CURRENT_SOURCE_DIR}/jidctint.c
	${CMAKE_CURRENT_SOURCE_DIR}/jidctred.c
	${CMAKE_CURRENT_SOURCE_DIR}/jquant1.c
	${CMAKE_CURRENT_SOURCE_DIR}/jquant2.c
	${CMAKE_CURRENT_SOURCE_DIR}/jutils.c
	${CMAKE_CURRENT_SOURCE_DIR}/jmemmgr.c
	${CMAKE_CURRENT_SOURCE_DIR}/jmemnobs.c)

if(WITH_ARITH_ENC OR WITH_ARITH_DEC)
  set(JPEG_SOURCES ${JPEG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/jaricom.c)
endif()

if(WITH_ARITH_ENC)
  set(JPEG_SOURCES ${JPEG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/jcarith.c)
endif()

if(WITH_ARITH_DEC)
  set(JPEG_SOURCES ${JPEG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/jdarith.c)
endif()

if(WITH_SIMD)
  add_definitions(-DWITH_SIMD)
  add_subdirectory(simd)
  if(SIMD_X86_64)
    set(JPEG_SOURCES ${JPEG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/simd/jsimd_x86_64.c)
  else()
    set(JPEG_SOURCES ${JPEG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/simd/jsimd_i386.c)
  endif()
  # This tells CMake that the "source" files haven't been generated yet
  set_source_files_properties(${SIMD_OBJS} PROPERTIES GENERATED 1)
else()
  set(JPEG_SOURCES ${JPEG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/jsimd_none.c)
  message(STATUS "Not using SIMD acceleration")
endif()

#add_subdirectory(sharedlib)

add_library(jpeg-static STATIC ${JPEG_SOURCES} ${SIMD_OBJS})
if(NOT MSVC)
  set_target_properties(jpeg-static PROPERTIES OUTPUT_NAME jpeg)
endif()
if(WITH_SIMD)
  add_dependencies(jpeg-static simd)
endif()

set(TURBOJPEG_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/turbojpeg.c
	${CMAKE_CURRENT_SOURCE_DIR}/transupp.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdatadst-tj.c
	${CMAKE_CURRENT_SOURCE_DIR}/jdatasrc-tj.c)

#add_library(turbojpeg SHARED ${TURBOJPEG_SOURCES})
#set_target_properties(turbojpeg PROPERTIES DEFINE_SYMBOL DLLDEFINE)
#if(MINGW)
#  set_target_properties(turbojpeg PROPERTIES LINK_FLAGS -Wl,--kill-at)
#endif()

#target_link_libraries(turbojpeg jpeg-static)
#set_target_properties(turbojpeg PROPERTIES LINK_INTERFACE_LIBRARIES "")

add_library(turbojpeg-static STATIC ${JPEG_SOURCES} ${SIMD_OBJS}
  ${CMAKE_CURRENT_SOURCE_DIR}/turbojpeg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/transupp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/jdatadst-tj.c
  ${CMAKE_CURRENT_SOURCE_DIR}/jdatasrc-tj.c)
  
if(NOT MSVC)
  set_target_properties(turbojpeg-static PROPERTIES OUTPUT_NAME turbojpeg)
endif()
if(WITH_SIMD)
  add_dependencies(turbojpeg-static simd)
endif()

#add_executable(tjunittest tjunittest.c tjutil.c)
#target_link_libraries(tjunittest turbojpeg)

#add_executable(tjunittest-static tjunittest.c tjutil.c)
#target_link_libraries(tjunittest-static turbojpeg-static)

#add_executable(tjbench tjbench.c bmp.c tjutil.c rdbmp.c rdppm.c wrbmp.c wrppm.c)
#target_link_libraries(tjbench turbojpeg jpeg-static)
#set_property(TARGET tjbench PROPERTY COMPILE_FLAGS
#  "-DBMP_SUPPORTED -DPPM_SUPPORTED")

#add_executable(tjbench-static tjbench.c bmp.c tjutil.c rdbmp.c rdppm.c wrbmp.c
#  wrppm.c)
#target_link_libraries(tjbench-static turbojpeg-static jpeg-static)
#set_property(TARGET tjbench-static PROPERTY COMPILE_FLAGS
#  "-DBMP_SUPPORTED -DPPM_SUPPORTED")

#add_executable(cjpeg-static cjpeg.c cdjpeg.c rdbmp.c rdgif.c rdppm.c rdswitch.c
#  rdtarga.c)
#set_property(TARGET cjpeg-static PROPERTY COMPILE_FLAGS
#  "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED -DUSE_SETMODE")
#target_link_libraries(cjpeg-static jpeg-static)

#add_executable(djpeg-static djpeg.c cdjpeg.c rdcolmap.c rdswitch.c wrbmp.c wrgif.c
#  wrppm.c wrtarga.c)
#set_property(TARGET djpeg-static PROPERTY COMPILE_FLAGS
#  "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED -DUSE_SETMODE")
#target_link_libraries(djpeg-static jpeg-static)

#add_executable(jpegtran-static jpegtran.c cdjpeg.c rdswitch.c transupp.c)
#target_link_libraries(jpegtran-static jpeg-static)
#set_property(TARGET jpegtran-static PROPERTY COMPILE_FLAGS "-DUSE_SETMODE")

#add_executable(rdjpgcom rdjpgcom.c)

#add_executable(wrjpgcom rdjpgcom.c)