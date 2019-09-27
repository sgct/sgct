# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(soundExample main.cpp)
set_compile_options(soundExample)
target_link_libraries(soundExample PRIVATE sgct)

find_library(ALUT_DEBUG_LIBRARY NAMES alutd libalutd libalut_cpp11d REQUIRED)
find_library(ALUT_RELEASE_LIBRARY NAMES alut libalut libalut_cpp11 REQUIRED)
set(ALUT_LIBRARY
  debug ${ALUT_DEBUG_LIBRARY}
  optimized ${ALUT_RELEASE_LIBRARY}
)
find_path(ALUT_INCLUDE_DIRECTORY NAMES AL/alut.h REQUIRED)

find_package(OpenAL REQUIRED)

target_include_directories(soundExample
  PRIVATE
    ${OPENAL_INCLUDE_DIRECTORY}
    ${ALUT_INCLUDE_DIRECTORY}
)
target_link_libraries(soundExample PRIVATE ${OPENAL_LIBRARY} ${ALUT_LIBRARY})

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/file1.wav"
)
copy_files(soundExample ${files})
copy_sgct_dynamic_libraries(soundExample)
set_target_properties(soundExample PROPERTIES FOLDER "Examples")
