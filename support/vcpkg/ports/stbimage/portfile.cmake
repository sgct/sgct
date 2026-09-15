# stbimage has no upstream registry port; This is a small library that compiles the
# stb_image/stb_image_write implementation exactly once so it can be shared without
# duplicate-symbol/ODR conflicts (see https://github.com/sgct/stbimage for details).

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO sgct/stbimage
  REF c498ec0828ecb392a6d788ab637d6db20efe0ac1
  SHA512 c6a2a333fb81b658903b3a857cfd03b9db2d6b764c2e736e24ac52fddf8eecf7b83080aa16f8e7798d7ca2b195fbc9bc0aaa6c04c4b7ddcf91d09cd2782eb917
  HEAD_REF master
)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(CONFIG_PATH share/stbimage)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
