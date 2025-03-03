##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2025                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(SGCTTest)

target_sources(
  SGCTTest
  PRIVATE
    test_config_examples.cpp
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
)

# Switching to cxx_std_23 triggers a bug in Clang17
# https://github.com/llvm/llvm-project/issues/61415
target_compile_features(SGCTTest PRIVATE cxx_std_20)
target_compile_definitions(SGCTTest PUBLIC BASE_PATH="${PROJECT_SOURCE_DIR}")

if (MSVC)
  target_compile_options(SGCTTest PRIVATE "-Od" "/bigobj")
else ()
  target_compile_options(SGCTTest PRIVATE "-O0")
endif ()

find_package(nlohmann_json REQUIRED)
find_package(glm REQUIRED)
find_package(Catch2 REQUIRED)

target_link_libraries(SGCTTest PRIVATE Catch2::Catch2WithMain sgct::sgct nlohmann_json::nlohmann_json nlohmann_json_schema_validator::nlohmann_json_schema_validator glm::glm)

if (APPLE)
  target_link_libraries(SGCTTest PRIVATE ${CARBON_LIBRARY} ${COREFOUNDATION_LIBRARY} ${COCOA_LIBRARY} ${APP_SERVICES_LIBRARY})
endif ()

add_test(NAME SGCTTest COMMAND SGCTTest)
