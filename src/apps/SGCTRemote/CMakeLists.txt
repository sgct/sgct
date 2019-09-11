# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SGCTRemote main.cpp)
set_compile_options(SGCTRemote)
target_link_libraries(SGCTRemote PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_remote.xml"
)
copy_files(SGCTRemote ${files})
copy_sgct_dynamic_libraries(SGCTRemote)

if (MSVC)
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/apps/SGCTRemote/SGCTRemote_GUI/SGCTRemoteGUI.csproj.in
    ${PROJECT_SOURCE_DIR}/src/apps/SGCTRemote/SGCTRemote_GUI/SGCTRemoteGUI.csproj
  )
  include_external_msproject(
    SGCTRemoteGUI
    ${PROJECT_SOURCE_DIR}/src/apps/SGCTRemote/SGCTRemote_GUI/SGCTRemoteGUI.csproj
  )
endif()
