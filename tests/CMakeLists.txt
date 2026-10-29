##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(SGCTTest)

target_sources(
  SGCTTest
  PRIVATE
    test_commandline.cpp
    test_config_examples.cpp
    test_correction_domeprojection.cpp
    test_correction_obj.cpp
    test_correction_paulbourke.cpp
    test_correction_pfm.cpp
    test_correction_simcad.cpp
    test_error.cpp
    test_image.cpp
    test_log.cpp
    test_config_load_capture.cpp
    test_config_load_cluster.cpp
    test_config_load_cubemapprojection.cpp
    test_config_load_cylindricalprojection.cpp
    test_config_load_equirectangularprojection.cpp
    test_config_load_fisheyeprojection.cpp
    test_config_load_generatorversion.cpp
    test_config_load_meta.cpp
    test_config_load_node.cpp
    test_config_load_planarprojection.cpp
    test_config_load_projectionplane.cpp
    test_config_load_scene.cpp
    test_config_load_settings.cpp
    test_config_load_sphericalmirrorprojection.cpp
    test_config_load_texturemappedprojection.cpp
    test_config_load_tracker.cpp
    test_config_load_user.cpp
    test_config_load_viewport.cpp
    test_config_load_window.cpp
    test_config_roundtrip.cpp
    test_config_serialize.cpp
    test_config_validate.cpp
    test_projection.cpp
    test_shareddata.cpp
    test_user.cpp
)

target_compile_features(SGCTTest PRIVATE cxx_std_23)
target_compile_definitions(SGCTTest PUBLIC BASE_PATH="${PROJECT_SOURCE_DIR}")
target_compile_definitions(
  SGCTTest
  PRIVATE
    $<$<BOOL:${SGCT_SCALABLE_SUPPORT}>:SGCT_HAS_SCALABLE>
)

if (MSVC)
  target_compile_options(SGCTTest PRIVATE "/Od" "/bigobj")
else ()
  target_compile_options(SGCTTest PRIVATE "-O0")
endif ()

find_package(glm REQUIRED)
find_package(Catch2 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(stbimage CONFIG REQUIRED)

target_link_libraries(
  SGCTTest
  PRIVATE
    sgct::sgct
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
    glm::glm
    unofficial::stbimage::stbimage-read-impl
)

include(Catch)
# Register each test case individually so that failures are reported per test case
catch_discover_tests(SGCTTest DISCOVERY_MODE PRE_TEST PROPERTIES LABELS unit)

add_test(
  NAME VcpkgManifestSync
  COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/support/vcpkg/check-manifest-sync.cmake
)
