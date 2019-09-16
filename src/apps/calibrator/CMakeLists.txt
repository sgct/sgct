# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(calibrator main.cpp Dome.h Dome.cpp)
set_compile_options(calibrator)
target_link_libraries(calibrator PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/correction_mesh.mpcdi"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/mpcdi_example1_distorted.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/mpcdi_example2_undistorted.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_mpcdi_example1.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_mpcdi_example2.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_PlanarProjection_trackedAspectRatio.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_Projectionplane_trackedAspectRatio.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_Projectionplane_unTrackedAspectRatio.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_PlanarProjection_tracked.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_Projectionplane_tracked.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_Projectionplane_unTracked.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/zero_distortion_mesh.mpcdi"
)
copy_files(calibrator ${files})
copy_sgct_dynamic_libraries(calibrator)
