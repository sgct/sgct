# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(stitcher main.cpp)
set_compile_options(stitcher)
target_link_libraries(stitcher PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/bottom.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/left.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/right.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_mono.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_stereo.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/top.png"
)
copy_files(stitcher ${files})
copy_sgct_dynamic_libraries(stitcher)
set_target_properties(stitcher PROPERTIES FOLDER "Examples")

if (MSVC)
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj.in
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj
  )
  include_external_msproject(Stitcher_GUI
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj
  )
  set_target_properties(Stitcher_GUI PROPERTIES FOLDER "Examples")
endif()
