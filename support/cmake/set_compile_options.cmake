##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

function (set_compile_options target)
  set_property(TARGET ${target} PROPERTY CXX_STANDARD 17)
  set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)
  if (MSVC)
    target_compile_options(
      ${target}
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
  elseif (NOT LINUX AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
      ${target}
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
      "-Wcomma"
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
      "-Wkeyword-macro"
      "-Wlanguage-extension-token"
      "-Wloop-analysis"
      "-Wmethod-signatures"
      "-Wmicrosoft-end-of-file"
      "-Wmicrosoft-enum-forward-reference"
      "-Wmicrosoft-fixed-enum"
      "-Wmicrosoft-flexible-array"
      "-Wmissing-noreturn"
      "-Wnewline-eof"
      "-Wnon-virtual-dtor"
      "-Wold-style-cast"
      "-Wpessimizing-move"
      "-Wpointer-arith"
      "-Wpragmas"
      "-Wredundant-move"
      "-Wshadow-field-in-constructor"
      "-Wshift-sign-overflow"
      "-Wshorten-64-to-32"
      "-Wstring-conversion"
      "-Wtautological-compare"
      "-Wthread-safety"
      "-Wundefined-reinterpret-cast"
      "-Wunneeded-internal-declaration"
      "-Wunneeded-member-function"
      "-Wunreachable-code-break"
      "-Wunreachable-code-loop-increment"
      "-Wunreachable-code-return"
      "-Wunused-exception-parameter"
      "-Wunused-label"
      "-Wunused-local-typedef"
      "-Wunused-macros"
      "-Wunused-private-field"
      "-Wunused-result"
      "-Wunused-variable"
      "-Wused-but-marked-unused"
      "-Wvariadic-macros"
      "-Wvla"
      "-Wzero-length-array"

      "-Wno-header-hygiene"
      "-Wno-missing-braces"
      "-Wno-unused-function"
    )
  elseif (LINUX AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
      ${target}
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
      "-Wcomma"
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
      "-Wkeyword-macro"
      "-Wlanguage-extension-token"
      "-Wloop-analysis"
      "-Wmethod-signatures"
      "-Wmicrosoft-end-of-file"
      "-Wmicrosoft-enum-forward-reference"
      "-Wmicrosoft-fixed-enum"
      "-Wmicrosoft-flexible-array"
      "-Wmissing-noreturn"
      "-Wnewline-eof"
      "-Wnon-virtual-dtor"
      "-Wold-style-cast"
      "-Wpessimizing-move"
      "-Wpointer-arith"
      "-Wpragmas"
      "-Wredundant-move"
      "-Wshadow-field-in-constructor"
      "-Wshift-sign-overflow"
      "-Wshorten-64-to-32"
      "-Wstring-conversion"
      "-Wtautological-compare"
      "-Wthread-safety"
      "-Wundefined-reinterpret-cast"
      "-Wunneeded-internal-declaration"
      "-Wunneeded-member-function"
      "-Wunreachable-code-break"
      "-Wunreachable-code-loop-increment"
      "-Wunreachable-code-return"
      "-Wunused-exception-parameter"
      "-Wunused-label"
      "-Wunused-local-typedef"
      "-Wunused-macros"
      "-Wunused-private-field"
      "-Wunused-result"
      "-Wunused-variable"
      "-Wused-but-marked-unused"
      "-Wvariadic-macros"
      "-Wvla"
      "-Wzero-length-array"

      "-Wno-header-hygiene"
      "-Wno-missing-braces"
      "-Wno-unused-function"
    )

    target_link_libraries(openspace-core PUBLIC "-lc++" "-lc++abi" "-lc++experimental")

  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(
      ${target}
      PRIVATE
      "-ggdb"
      "-Wall"
      "-Wextra"
      "-Wpedantic"
      "-Walloc-zero"
      "-Wcast-qual"
      "-Wdate-time"
      "-Wduplicated-cond"
      "-Wlogical-op"
      "-Wno-long-long"
      "-Wno-write-strings"
      "-Wnon-virtual-dtor"
      "-Wold-style-cast"
      "-Woverloaded-virtual"
      "-Wshadow"
      "-Wsuggest-attribute=const"
      "-Wsuggest-final-types"
      "-Wsuggest-final-methods"
      "-Wsuggest-override"
      "-Wundef"
      "-Wuseless-cast"
      "-Wzero-as-null-pointer-constant"
    )
  endif ()
endfunction ()
