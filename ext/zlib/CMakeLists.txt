# (abock, 2022-07-04) Modified the required version to suppress CMake deprecation warning
cmake_minimum_required(VERSION 2.9.0)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

# (aobkc, 2022-07-04) Added the project version to suppress policy CMP0048 warning
cmake_policy(SET CMP0048 NEW)
project(zlib LANGUAGES C VERSION 1.2.8)

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(INSTALL_MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man")
set(INSTALL_PKGCONFIG_DIR "${CMAKE_INSTALL_PREFIX}/share/pkgconfig")

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
enable_testing()

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
if(HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
endif()
if(HAVE_STDDEF_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
endif()
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
	
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	
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

	if(NOT "${MSVC_VERSION}" LESS 1400)
		add_definitions( "/MP" )
	endif()
endif()

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    # If we're doing an out of source build and the user has a zconf.h
    # in their source tree...
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h)
        message(STATUS "Renaming")
        message(STATUS "    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h")
        message(STATUS "to 'zconf.h.included' because this file is included with zlib")
        message(STATUS "but CMake generates it automatically in the build directory.")
        file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.included)
  endif()
endif()

set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein
		${ZLIB_PC} @ONLY)
configure_file(	${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein
		${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})


#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

#if(NOT MINGW)
#    set(ZLIB_SRCS ${ZLIB_SRCS}
#        win32/zlib1.rc # If present will override custom build rule below.
#    )
#endif()

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

#if( MINGW )
#    # This gets us DLL resource information when compiling on MinGW.
#    if(NOT CMAKE_RC_COMPILER)
#        SET(CMAKE_RC_COMPILER windres.exe)
#    endif()
#
#    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
#                       COMMAND ${CMAKE_RC_COMPILER}
#                            -D GCC_WINDRES
#                            -I ${CMAKE_CURRENT_SOURCE_DIR}
#                            -I ${CMAKE_CURRENT_BINARY_DIR}
#                            -o ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
#                            -i ${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib1.rc)
#    set(ZLIB_SRCS ${ZLIB_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj)
#endif( MINGW )


if (BUILD_SHARED_LIBS)
    add_library(zlib1 SHARED ${ZLIB_SRCS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
    set_target_properties(zlib1 PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
    set_target_properties(zlib1 PROPERTIES SOVERSION 1)
else()
    add_library(zlib1 STATIC ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
endif()
# Define same target as find_package(ZLIB)
add_library(ZLIB::ZLIB ALIAS zlib1)
target_include_directories(zlib1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(MINIZIP_PATH contrib/minizip)
set(MINIZIP_SRCS
  ${MINIZIP_PATH}/ioapi.c
  ${MINIZIP_PATH}/unzip.c
  ${MINIZIP_PATH}/zip.c
  ${MINIZIP_PATH}/mztools.c
)
if(WIN32)
  list(APPEND MINIZIP_SRCS ${MINIZIP_PATH}/iowin32.c)
endif()

set(MINIZIP_PUBLIC_HDRS
  ${MINIZIP_PATH}/crypt.h
  ${MINIZIP_PATH}/ioapi.h
  ${MINIZIP_PATH}/unzip.h
  ${MINIZIP_PATH}/zip.h
  ${MINIZIP_PATH}/mztools.h
)
if(WIN32)
  list(APPEND MINIZIP_PUBLIC_HDRS ${MINIZIP_PATH}/iowin32.h)
endif()

add_library(minizip STATIC ${MINIZIP_SRCS} ${MINIZIP_PUBLIC_HDRS})
add_library(minizip::minizip ALIAS minizip)

target_link_libraries(minizip PRIVATE ZLIB::ZLIB)
target_compile_definitions(minizip PRIVATE -D_ZLIB_H)
target_include_directories(minizip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/contrib/)

if(UNIX)
	# On unix-like platforms the library is almost always called libz
	set_target_properties(zlib1 PROPERTIES OUTPUT_NAME z)
	set_target_properties(minizip PROPERTIES OUTPUT_NAME minizip)
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
    install(TARGETS zlib1
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )
    install(TARGETS minizip
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )
endif()
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
    install(FILES zlib.3 DESTINATION "${INSTALL_MAN_DIR}/man3")
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
    install(FILES ${ZLIB_PC} DESTINATION "${INSTALL_PKGCONFIG_DIR}")
endif()
