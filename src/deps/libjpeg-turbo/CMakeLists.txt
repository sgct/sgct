#
# Setup
#

cmake_minimum_required(VERSION 2.8.7)
#cmake_policy(SET CMP0022 OLD)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

project(libjpeg-turbo)
set(VERSION 1.4.1)

if(WIN32)
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
  execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
  string(REGEX REPLACE "\n" "" BUILD "${BUILD}")
  #message(FATAL_ERROR "Platform not supported by this build system.  Use autotools instead.")
endif()
	
#find size of size_t
include (CheckTypeSize)
check_type_size("size_t" SIZEOF_SIZE_T)
if(SIZEOF_SIZE_T)
#message(STATUS "Size of size_t=" ${SIZEOF_SIZE_T})
add_definitions(-DSIZEOF_SIZE_T=${SIZEOF_SIZE_T})
endif()

option(JPEG_TURBO_FORCE32bit "Force 32-bit" FALSE)

option(JPEG_TURBO_WITH_SIMD "Include SIMD extensions" TRUE)
option(JPEG_TURBO_WITH_ARITH_ENC "Include arithmetic encoding support" TRUE)
option(JPEG_TURBO_WITH_ARITH_DEC "Include arithmetic decoding support" TRUE)
option(JPEG_TURBO_WITH_JPEG7 "Emulate libjpeg v7 API/ABI (this makes libjpeg-turbo backward incompatible with libjpeg v6b)" FALSE)
option(JPEG_TURBO_WITH_JPEG8 "Emulate libjpeg v8 API/ABI (this makes libjpeg-turbo backward incompatible with libjpeg v6b)" FALSE)
option(JPEG_TURBO_WITH_MEM_SRCDST "Include in-memory source/destination manager functions when emulating the libjpeg v6b or v7 API/ABI" TRUE)
option(JPEG_TURBO_WITH_TURBOJPEG "Include the TurboJPEG wrapper library and associated test programs" TRUE)
option(JPEG_TURBO_WITH_12BIT "Encode/decode JPEG images with 12-bit samples (implies JPEG_TURBO_WITH_SIMD=0 JPEG_TURBO_WITH_TURBOJPEG=0 JPEG_TURBO_WITH_ARITH_ENC=0 JPEG_TURBO_WITH_ARITH_DEC=0)" FALSE)
set(ENABLE_STATIC TRUE)

if(JPEG_TURBO_WITH_12BIT)
  set(JPEG_TURBO_WITH_SIMD FALSE)
  set(JPEG_TURBO_WITH_TURBOJPEG FALSE)
  set(JPEG_TURBO_WITH_ARITH_ENC FALSE)
  set(JPEG_TURBO_WITH_ARITH_DEC FALSE)
  set(BITS_IN_JSAMPLE 12)
  message(STATUS "12-bit JPEG support enabled")
else()
  set(BITS_IN_JSAMPLE 8)
endif()

if(JPEG_TURBO_WITH_ARITH_ENC)
  set(C_ARITH_CODING_SUPPORTED 1)
  message(STATUS "Arithmetic encoding support enabled")
else()
  message(STATUS "Arithmetic encoding support disabled")
endif()

if(JPEG_TURBO_WITH_ARITH_DEC)
  set(D_ARITH_CODING_SUPPORTED 1)
  message(STATUS "Arithmetic decoding support enabled")
else()
  message(STATUS "Arithmetic decoding support disabled")
endif()

if(JPEG_TURBO_WITH_TURBOJPEG)
  message(STATUS "TurboJPEG C wrapper enabled")
else()
  message(STATUS "TurboJPEG C wrapper disabled")
endif()

set(SO_AGE 0)
if(JPEG_TURBO_WITH_MEM_SRCDST)
  set(SO_AGE 1)
endif()

set(JPEG_LIB_VERSION 62)
set(DLL_VERSION ${JPEG_LIB_VERSION})
set(FULLVERSION ${DLL_VERSION}.${SO_AGE}.0)
if(JPEG_TURBO_WITH_JPEG8)
  set(JPEG_LIB_VERSION 80)
  set(DLL_VERSION 8)
  set(FULLVERSION ${DLL_VERSION}.0.2)
  message(STATUS "Emulating libjpeg v8 API/ABI")
elseif(JPEG_TURBO_WITH_JPEG7)
  set(JPEG_LIB_VERSION 70)
  set(DLL_VERSION 7)
  set(FULLVERSION ${DLL_VERSION}.${SO_AGE}.0)
  message(STATUS "Emulating libjpeg v7 API/ABI")
endif(JPEG_TURBO_WITH_JPEG8)

if(JPEG_TURBO_WITH_MEM_SRCDST)
  set(MEM_SRCDST_SUPPORTED 1)
  message(STATUS "In-memory source/destination managers enabled")
else()
  message(STATUS "In-memory source/destination managers disabled")
endif()

# Detect whether compiler is 64-bit
if(NOT JPEG_TURBO_FORCE32bit)
	if(MSVC AND CMAKE_CL_64)
	  set(SIMD_X86_64 1)
	  set(64BIT 1)
	elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
	  set(SIMD_X86_64 1)
	  set(64BIT 1)
	endif()
endif()

if(64BIT)
  message(STATUS "64-bit build")
else()
  message(STATUS "32-bit build")
endif()

configure_file(win/jconfig.h.in jconfig.h)
configure_file(win/jconfigint.h.in jconfigint.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Targets
#

set(JPEG_SOURCES jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c
  jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c jcphuff.c
  jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c jdatasrc.c
  jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c
  jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c jerror.c
  jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c
  jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c)

if( MSVC )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT.lib")
	
	if (NOT SGCT_USE_MSVC_RUNTIMES)
        foreach (flag CMAKE_C_FLAGS
                        CMAKE_C_FLAGS_DEBUG
                        CMAKE_C_FLAGS_RELEASE
                        CMAKE_C_FLAGS_MINSIZEREL
                        CMAKE_C_FLAGS_RELWITHDEBINFO)

            if (${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
            if (${flag} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
            endif()

        endforeach()
    endif()
	
	add_definitions(-W3 -wd4996)
endif()
  
if(JPEG_TURBO_WITH_ARITH_ENC OR JPEG_TURBO_WITH_ARITH_DEC)
  set(JPEG_SOURCES ${JPEG_SOURCES} jaricom.c)
endif()

if(JPEG_TURBO_WITH_ARITH_ENC)
  set(JPEG_SOURCES ${JPEG_SOURCES} jcarith.c)
endif()

if(JPEG_TURBO_WITH_ARITH_DEC)
  set(JPEG_SOURCES ${JPEG_SOURCES} jdarith.c)
endif()

if(JPEG_TURBO_WITH_SIMD)
  add_definitions(-DWITH_SIMD)
  add_subdirectory(simd)
  if(SIMD_X86_64)
    set(JPEG_SOURCES ${JPEG_SOURCES} simd/jsimd_x86_64.c)
  else()
    set(JPEG_SOURCES ${JPEG_SOURCES} simd/jsimd_i386.c)
  endif()
  # This tells CMake that the "source" files haven't been generated yet
  set_source_files_properties(${SIMD_OBJS} PROPERTIES GENERATED 1)
else()
  set(JPEG_SOURCES ${JPEG_SOURCES} jsimd_none.c)
  message(STATUS "Not using SIMD acceleration")
endif()

#if(ENABLE_STATIC OR JPEG_TURBO_WITH_TURBOJPEG)
#  add_library(jpeg-static STATIC ${JPEG_SOURCES} ${SIMD_OBJS})
#  if(NOT MSVC)
#    set_target_properties(jpeg-static PROPERTIES OUTPUT_NAME jpeg)
#  endif()
#  if(JPEG_TURBO_WITH_SIMD)
#    add_dependencies(jpeg-static simd)
#  endif()
#endif()

if(JPEG_TURBO_WITH_TURBOJPEG)
  set(TURBOJPEG_SOURCES turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c)

  if(ENABLE_STATIC)
    add_library(turbojpeg-static STATIC ${JPEG_SOURCES} ${SIMD_OBJS}
      turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c)
    if(NOT MSVC)
      set_target_properties(turbojpeg-static PROPERTIES OUTPUT_NAME turbojpeg)
    endif()
    if(JPEG_TURBO_WITH_SIMD)
      add_dependencies(turbojpeg-static simd)
    endif()
  endif()
endif()

if(JPEG_TURBO_WITH_12BIT)
  set(COMPILE_FLAGS "-DGIF_SUPPORTED -DPPM_SUPPORTED -DUSE_SETMODE")
else()
  set(COMPILE_FLAGS "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED -DUSE_SETMODE")
	set(CJPEG_BMP_SOURCES rdbmp.c rdtarga.c)
	set(DJPEG_BMP_SOURCES wrbmp.c wrtarga.c)
endif()