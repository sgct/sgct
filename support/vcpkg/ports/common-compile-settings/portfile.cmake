##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO OpenSpace/common-compile-settings
  REF 07ba7694726ba84aa8409549f524cad0d6e5dfa5
  SHA512 af6c71c54ef9c1e5d1f23537e33686a6f86eb79685f5cf38f1062853460302b23aae7e296c40c364b2cbda9b581d2f7e6d3cfd950f57c19d750232355754bd86
  HEAD_REF master
)

# The repository only consists of CMake scripts and has no build of its own, so they are
# installed as they are, next to a config file that makes them available to find_package
file(INSTALL
  "${SOURCE_PATH}/common-compile-settings.cmake"
  "${SOURCE_PATH}/platforms"
  "${CMAKE_CURRENT_LIST_DIR}/common-compile-settingsConfig.cmake"
  "${CMAKE_CURRENT_LIST_DIR}/usage"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
