# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(calibrator main.cpp Dome.h Dome.cpp)
target_link_libraries(calibrator PRIVATE sgct)

set_property(TARGET calibrator PROPERTY CXX_STANDARD 17)
set_property(TARGET calibrator PROPERTY CXX_STANDARD_REQUIRED ON)

option(SGCT_COPY_EXAMPLE_RESOURCES "Copy example resources" ON)
set(EXAMPLE_TARGET_PATH ${SGCT_EXAMPLE_OUTPUT_DIR}/calibrator)

set_target_properties(calibrator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXAMPLE_TARGET_PATH}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXAMPLE_TARGET_PATH}
    FOLDER "Examples"
)

# copy data
if (SGCT_COPY_EXAMPLE_RESOURCES)
    file(GLOB files
        "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.jpg"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.xml"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.mpcdi"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert*"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag*"
    )
    file(COPY ${files} DESTINATION ${EXAMPLE_TARGET_PATH})
    if (WIN32)
        file(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/*.bat")
        file(COPY ${scripts} DESTINATION ${EXAMPLE_TARGET_PATH})
    elseif (UNIX)
        file(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/*.command")
        file(COPY ${scripts} DESTINATION ${EXAMPLE_TARGET_PATH})
    endif()
endif()

find_package(OpenGL REQUIRED)

if (WIN32)
    target_link_libraries(calibrator PRIVATE ${OPENGL_gl_LIBRARY} ws2_32)
elseif (APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

    target_link_libraries(calibrator PRIVATE
        ${OPENGL_gl_LIBRARY} ${COCOA_LIBRARY} ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
else() #linux
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)

    target_link_libraries(calibrator PRIVATE
        ${OPENGL_gl_LIBRARY} ${X11_X11_LIB} ${X11_Xrandr_LIB}
        ${X11_Xinerama_LIB} ${X11_Xinput_LIB} ${X11_Xxf86vm_LIB} ${X11_Xcursor_LIB}
        ${CMAKE_THREAD_LIBS_INIT}
    )
endif()

if (MSVC)
    target_compile_options(
        calibrator
        PRIVATE
        "/ZI"       # Edit and continue support
        "/MP"       # Multi-threading support
        "/W4"       # Highest warning level
        "/wd4201"   # nonstandard extension used : nameless struct/union
        "/wd4505"   # unreferenced local function has been removed
        "/std:c++17"
        "/permissive-"
        "/Zc:strictStrings-"    # Windows header don't adhere to this
        "/Zc:__cplusplus" # Correctly set the __cplusplus macro
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        calibrator
        PRIVATE
        "-stdlib=libc++"
        "-Wall"
        "-Wextra"
        "-Wabstract-vbase-init"
        "-Warray-bounds-pointer-arithmetic"
        "-Wassign-enum"
        "-Wauto-import"
        "-Wbad-function-cast"
        "-Wbitfield-constant-conversion"
        "-Wcast-calling-convention"
        "-Wcast-qual"
        "-Wchar-subscripts"
        "-Wcomma"
        "-Wcomment"
        "-Wcomplex-component-init"
        "-Wconditional-uninitialized"
        "-Wdate-time"
        "-Wdeprecated-implementations"
        "-Wdollar-in-identifier-extension"
        "-Wduplicate-enum"
        "-Wduplicate-method-match"
        "-Wempty-body"
        "-Wformat-pedantic"
        "-Wheader-hygiene"
        "-Widiomatic-parentheses"
        "-Wimplicit-fallthrough"
        "-Wimport-preprocessor-directive-pedantic"
        "-Winconsistent-missing-override"
        "-Winfinite-recursion"
        "-Wkeyword-macro"
        "-Wlanguage-extension-token"
        "-Wloop-analysis"
        "-Wmethod-signatures"
        "-Wmicrosoft-end-of-file"
        "-Wmicrosoft-enum-forward-reference"
        "-Wmicrosoft-fixed-enum"
        "-Wmicrosoft-flexible-array"
        "-Wmismatched-tags"
        "-Wmissing-field-initializers"
        "-Wmissing-noreturn"
        "-Wnewline-eof"
        "-Wnon-virtual-dtor"
        "-Wold-style-cast"
        "-Woverloaded-virtual"
        "-Wpessimizing-move"
        "-Wpointer-arith"
        "-Wpragmas"
        "-Wredundant-move"
        "-Wreorder"
        "-Wsemicolon-before-method-body"
        "-Wshadow-field-in-constructor"
        "-Wshift-sign-overflow"
        "-Wshorten-64-to-32"
        "-Wsign-compare"
        "-Wstring-conversion"
        "-Wtautological-compare"
        "-Wthread-safety"
        "-Wundef"
        "-Wundefined-reinterpret-cast"
        "-Wuninitialized"
        "-Wunneeded-internal-declaration"
        "-Wunneeded-member-function"
        "-Wunreachable-code-break"
        "-Wunreachable-code-loop-increment"
        "-Wunreachable-code-return"
        "-Wunused-exception-parameter"
        "-Wunused-label"
        "-Wunused-local-typedef"
        "-Wunused-macros"
        "-Wunused-parameter"
        "-Wunused-private-field"
        "-Wunused-result"
        "-Wunused-variable"
        "-Wused-but-marked-unused"
        "-Wvariadic-macros"
        "-Wvla"
        "-Wzero-length-array"
        "-Wno-missing-braces"
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(
        calibrator
        PRIVATE
        "-ggdb"
        "-Wall"
        "-Wextra"
        "-Wpedantic"
        "-Wunused-parameter"
        "-Wuninitialized"
        "-Wsuggest-attribute=const"
        "-Wsuggest-final-types"
        "-Wsuggest-final-methods"
        "-Wsuggest-override"
        "-Walloc-zero"
        "-Wduplicated-cond"
        "-Wshadow"
        "-Wundef"
        "-Wcast-qual"
        "-Wzero-as-null-pointer-constant"
        "-Wdate-time"
        "-Wuseless-cast"
        "-Wlogical-op"
        "-Wint-in-bool-context"

        "-Wno-write-strings"
        "-Wnon-virtual-dtor"
        "-Wold-style-cast"
        "-Woverloaded-virtual"
        "-Wno-long-long"
    )
endif ()
