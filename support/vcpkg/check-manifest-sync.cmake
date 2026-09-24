##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

# Verifies that the `sgct` overlay port under support/vcpkg/ports declares the same
# runtime dependencies and optional features as the root vcpkg.json. A superproject that
# adds SGCT through add_subdirectory cannot use SGCT's manifest, since vcpkg only reads
# the manifest of the top-level project, and instead depends on the port, so the two lists
# have to stay identical apart from the intentional differences listed below.
#
# Intentional differences (a mismatch in these is allowed, anything else is an error):
#   - the port additionally depends on the vcpkg-cmake / vcpkg-cmake-config host tools,
#     which the standalone manifest gets from vcpkg itself
#   - the port never builds the unit tests or Tracy support, so it omits their Catch2 and
#     Tracy dependencies
#
# Run with:  cmake -P support/vcpkg/check-manifest-sync.cmake

cmake_minimum_required(VERSION 3.19)

get_filename_component(SGCT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(MANIFEST "${SGCT_ROOT_DIR}/vcpkg.json")
set(PORT "${CMAKE_CURRENT_LIST_DIR}/ports/sgct/vcpkg.json")

# Dependencies that are allowed to appear only in the port
set(PORT_ONLY_DEPENDENCIES "vcpkg-cmake" "vcpkg-cmake-config")
# Dependencies that are allowed to appear only in the manifest
set(MANIFEST_ONLY_DEPENDENCIES "catch2" "tracy")

function (flatten_dependencies outVar dependencies)
  set(result "")
  string(JSON count LENGTH "${dependencies}")
  if (count GREATER 0)
    math(EXPR lastIndex "${count} - 1")
    foreach (i RANGE ${lastIndex})
      string(JSON entryType TYPE "${dependencies}" ${i})
      string(JSON entry GET "${dependencies}" ${i})

      if (entryType STREQUAL "STRING")
        list(APPEND result "${entry}")
        continue ()
      endif ()

      string(JSON name GET "${entry}" "name")

      string(JSON features ERROR_VARIABLE featuresError GET "${entry}" "features")
      set(featureList "")
      if (featuresError STREQUAL "NOTFOUND")
        string(JSON featureCount LENGTH "${features}")
        if (featureCount GREATER 0)
          math(EXPR lastFeature "${featureCount} - 1")
          foreach (j RANGE ${lastFeature})
            string(JSON feature GET "${features}" ${j})
            list(APPEND featureList "${feature}")
          endforeach ()
          list(SORT featureList)
        endif ()
      endif ()
      list(JOIN featureList "+" featureText)

      string(JSON defaults ERROR_VARIABLE defaultsError GET "${entry}" "default-features")
      if (NOT defaultsError STREQUAL "NOTFOUND")
        set(defaults "ON")
      endif ()

      string(JSON host ERROR_VARIABLE hostError GET "${entry}" "host")
      if (NOT hostError STREQUAL "NOTFOUND")
        set(host "OFF")
      endif ()

      string(JSON platform ERROR_VARIABLE platformError GET "${entry}" "platform")
      if (NOT platformError STREQUAL "NOTFOUND")
        set(platform "")
      endif ()

      list(APPEND result "${name}[${featureText}](default-features=${defaults})(host=${host})(platform=${platform})")
    endforeach ()
  endif ()

  list(SORT result)
  set(${outVar} "${result}" PARENT_SCOPE)
endfunction ()

# Removes every flattened entry whose dependency name is in `excluded` from `list`
function (drop_dependencies outVar list excluded)
  set(result "")
  foreach (entry IN LISTS list)
    string(REGEX REPLACE "\\[.*$" "" entryName "${entry}")
    if (NOT entryName IN_LIST excluded)
      list(APPEND result "${entry}")
    endif ()
  endforeach ()
  set(${outVar} "${result}" PARENT_SCOPE)
endfunction ()

function (feature_names outVar features)
  string(JSON count LENGTH "${features}")
  set(result "")
  if (count GREATER 0)
    math(EXPR lastIndex "${count} - 1")
    foreach (i RANGE ${lastIndex})
      string(JSON name MEMBER "${features}" ${i})
      list(APPEND result "${name}")
    endforeach ()
    list(SORT result)
  endif ()
  set(${outVar} "${result}" PARENT_SCOPE)
endfunction ()

# Returns the "dependencies" array of `feature`, or "[]" if the feature declares none
function (feature_dependencies outVar features feature)
  string(JSON deps ERROR_VARIABLE depsError GET "${features}" "${feature}" "dependencies")
  if (NOT depsError STREQUAL "NOTFOUND")
    set(deps "[]")
  endif ()
  set(${outVar} "${deps}" PARENT_SCOPE)
endfunction ()

file(READ "${MANIFEST}" manifestJson)
file(READ "${PORT}" portJson)

set(errors "")

string(JSON manifestDependencies GET "${manifestJson}" "dependencies")
string(JSON portDependencies GET "${portJson}" "dependencies")
flatten_dependencies(manifestDeps "${manifestDependencies}")
flatten_dependencies(portDeps "${portDependencies}")
drop_dependencies(manifestDeps "${manifestDeps}" "${MANIFEST_ONLY_DEPENDENCIES}")
drop_dependencies(portDeps "${portDeps}" "${PORT_ONLY_DEPENDENCIES}")
if (NOT manifestDeps STREQUAL portDeps)
  list(APPEND errors "  dependencies\n    vcpkg.json:  ${manifestDeps}\n    port:        ${portDeps}")
endif ()

string(JSON manifestFeatures GET "${manifestJson}" "features")
string(JSON portFeatures GET "${portJson}" "features")
feature_names(manifestFeatureNames "${manifestFeatures}")
feature_names(portFeatureNames "${portFeatures}")
if (NOT manifestFeatureNames STREQUAL portFeatureNames)
  list(APPEND errors "  feature names\n    vcpkg.json:  ${manifestFeatureNames}\n    port:        ${portFeatureNames}")
else ()
  foreach (feature ${manifestFeatureNames})
    feature_dependencies(manifestFeatureDependencies "${manifestFeatures}" "${feature}")
    feature_dependencies(portFeatureDependencies "${portFeatures}" "${feature}")
    flatten_dependencies(manifestFeatureDeps "${manifestFeatureDependencies}")
    flatten_dependencies(portFeatureDeps "${portFeatureDependencies}")
    if (NOT manifestFeatureDeps STREQUAL portFeatureDeps)
      list(APPEND errors "  feature '${feature}'\n    vcpkg.json:  ${manifestFeatureDeps}\n    port:        ${portFeatureDeps}")
    endif ()
  endforeach ()
endif ()

if (errors)
  list(JOIN errors "\n" errorText)
  message(FATAL_ERROR
    "vcpkg.json and support/vcpkg/ports/sgct/vcpkg.json have diverged:\n${errorText}\n"
  )
endif ()

message(STATUS "vcpkg.json and the sgct port are in sync")
