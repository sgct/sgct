function (copy_files target)
  # Add the copy command
  foreach(file_i ${ARGN})
    add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file_i}" $<TARGET_FILE_DIR:${target}>
  )
  endforeach ()
endfunction ()
