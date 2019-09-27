# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(FMOD_example_opengl3 main.cpp fmod_audio.cpp fmod_audio.h)
set_compile_options(FMOD_example_opengl3)
target_link_libraries(FMOD_example_opengl3 PRIVATE sgct)

find_package(FMOD REQUIRED)
target_include_directories(FMOD_example_opengl3 PRIVATE ${FMOD_INCLUDE_DIR})
target_link_libraries(FMOD_example_opengl PRIVATE ${FMOD_LIBRARY})

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/drumloop.wav"
  "${CMAKE_CURRENT_SOURCE_DIR}/jaguar.wav"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
)
copy_files(FMOD_example_opengl3 ${files})
copy_sgct_dynamic_libraries(FMOD_example_opengl3)
set_target_properties(FMOD_example_opengl3 PROPERTIES FOLDER "Examples")
