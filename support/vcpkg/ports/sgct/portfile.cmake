##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

# This port lives inside the SGCT repository and builds the enclosing checkout. When
# publishing SGCT to a registry, replace this with vcpkg_from_github(REPO sgct/sgct
# REF <tag> SHA512 <hash>) so that the port is reproducible and content-addressed.
get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

vcpkg_check_features(
  OUT_FEATURE_OPTIONS FEATURE_OPTIONS
  FEATURES
    ndi      SGCT_NDI_SUPPORT
    scalable SGCT_SCALABLE_SUPPORT
    tracy    SGCT_TRACY_SUPPORT
)

vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS
    ${FEATURE_OPTIONS}
    # /ZI is a developer convenience that would otherwise be baked into the shipped library
    -DSGCT_ENABLE_EDIT_CONTINUE=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME sgct CONFIG_PATH share/sgct)
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE
  "${CURRENT_PACKAGES_DIR}/debug/include"
  "${CURRENT_PACKAGES_DIR}/debug/share"
)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
