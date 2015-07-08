# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8)
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

project(freetype)

set(VERSION_MAJOR "2")
set(VERSION_MINOR "5")
set(VERSION_PATCH "2")
set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# Compiler definitions for building the library
add_definitions(-DFT2_BUILD_LIBRARY)

# Specify library include directories
include_directories(
				"${SGCT_SOURCE_PATH}/src/deps/zlib"
				"${PROJECT_SOURCE_DIR}/include"
				"${SGCT_BINARY_PATH}/src/deps/lpng/zlib") #needed for zconf

# Create the configuration file
message(STATUS "Creating directory, ${PROJECT_BINARY_DIR}/include.")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)

# For the auto-generated ftconfig.h file
include_directories("${PROJECT_BINARY_DIR}/include")
message(STATUS "Creating ${PROJECT_BINARY_DIR}/include/ftconfig.h.")
execute_process(
  COMMAND sed -e "s/FT_CONFIG_OPTIONS_H/<ftoption.h>/" -e "s/FT_CONFIG_STANDARD_LIBRARY_H/<ftstdlib.h>/" -e "s?/undef ?#undef ?"
  INPUT_FILE ${PROJECT_SOURCE_DIR}/builds/unix/ftconfig.in
  OUTPUT_FILE ${PROJECT_BINARY_DIR}/include/ftconfig.h
)

set(BASE_SRCS
  src/autofit/autofit.c
  src/base/ftadvanc.c
  src/base/ftbbox.c
  src/base/ftbitmap.c
  src/base/ftcalc.c
  src/base/ftcid.c
  src/base/ftdbgmem.c
  src/base/ftdebug.c
  src/base/ftfstype.c
  src/base/ftgasp.c
  src/base/ftgloadr.c
  src/base/ftglyph.c
  src/base/ftgxval.c
  src/base/ftinit.c
  src/base/ftlcdfil.c
  src/base/ftmm.c
  src/base/ftobjs.c
  src/base/ftotval.c
  src/base/ftoutln.c
  src/base/ftpatent.c
  src/base/ftpfr.c
  src/base/ftrfork.c
  src/base/ftsnames.c
  src/base/ftstream.c
  src/base/ftstroke.c
  src/base/ftsynth.c
  src/base/ftsystem.c
  src/base/fttrigon.c
  src/base/fttype1.c
  src/base/ftutil.c
  src/base/ftwinfnt.c
  src/base/ftxf86.c
  src/bdf/bdf.c
  src/bzip2/ftbzip2.c
  src/cache/ftcache.c
  src/cff/cff.c
  src/cid/type1cid.c
  src/gzip/ftgzip.c
  src/lzw/ftlzw.c
  src/pcf/pcf.c
  src/pfr/pfr.c
  src/psaux/psaux.c
  src/pshinter/pshinter.c
  src/psnames/psmodule.c
  src/raster/raster.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/type42/type42.c
  src/winfonts/winfnt.c
)

include_directories("src/truetype")
include_directories("src/sfnt")
include_directories("src/autofit")
include_directories("src/smooth")
include_directories("src/raster")
include_directories("src/psaux")
include_directories("src/psnames")

if( MSVC )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT.lib")
	
	if (NOT SGCT_USE_MSVC_RUNTIMES)
        foreach (flag ${SGCTCompilerFlags})

            if (${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
            if (${flag} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
            endif()

        endforeach()
    endif()
	
	foreach(CompilerFlag ${SGCTCompilerFlags})
		string(REPLACE "/Zi" "/Z7" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endif()

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

if( MSVC )
	#disable pdb-file debugging
	#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
endif()


add_library(freetype ${BASE_SRCS})