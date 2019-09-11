# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(stitcher main.cpp)
set_compile_options(stitcher)
target_link_libraries(stitcher PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(stitcher ${files})
copy_sgct_dynamic_libraries(stitcher)

if (MSVC)
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj.in
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj
  )
  include_external_msproject(Stitcher_GUI
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj
  )
endif()
