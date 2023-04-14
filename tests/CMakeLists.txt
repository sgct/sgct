##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(SGCTTest)

target_sources(
  SGCTTest
  PRIVATE
    equality.cpp

    test_config_load.cpp
    test_config_parse.cpp
    test_config_required_parameters.cpp
    test_config_required_parameters_schema.cpp
    test_config_roundtrip.cpp
)

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
