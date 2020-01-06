##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(remote main.cpp)
set_compile_options(remote)
target_link_libraries(remote PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_remote.xml"
)
copy_files(remote ${files})
copy_sgct_dynamic_libraries(remote)
set_target_properties(remote PROPERTIES FOLDER "Examples")

if (MSVC)
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/apps/remote/SGCTRemote_GUI/SGCTRemoteGUI.csproj.in
    ${PROJECT_SOURCE_DIR}/src/apps/remote/SGCTRemote_GUI/SGCTRemoteGUI.csproj
  )
  include_external_msproject(
    SGCTRemoteGUI
    ${PROJECT_SOURCE_DIR}/src/apps/remote/SGCTRemote_GUI/SGCTRemoteGUI.csproj
  )
  set_target_properties(SGCTRemoteGUI PROPERTIES FOLDER "Examples")
endif()
