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
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
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
      ${target}
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
endfunction ()