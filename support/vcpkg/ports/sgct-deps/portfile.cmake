##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

# sgct-deps installs nothing; its only purpose is to pull in the dependencies declared in
# the accompanying vcpkg.json so that consumers only ever have to name "sgct-deps".
set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright"
  "sgct-deps is a metapackage that contains no code of its own.\n"
  "See the licenses of the individual dependencies it pulls in.\n"
)
