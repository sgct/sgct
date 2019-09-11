# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt


function (copy_sgct_dynamic_libraries target)
  set(libs "")
  get_target_property(libs sgct DYNAMIC_LIBS)
  foreach (i ${libs})
    copy_files(${target} ${i})
  endforeach ()
endfunction()
