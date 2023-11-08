function(sgct_register_package name)
  set(incdirs "")
  foreach(target ${ARGN})
    get_target_property(dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    set(incdirs "${incdirs} ${dirs}")
  endforeach()
  string(TOLOWER "${name}" lowercase_name)
  set(${name}_DIR "${CMAKE_BINARY_DIR}/pkg/${lowercase_name}" CACHE PATH "" FORCE)
  file(WRITE "${CMAKE_BINARY_DIR}/pkg/${lowercase_name}/${name}Config.cmake"
    "# Fake Config file for ${name}\n"
    "set(${name}_FOUND ON)\n"
    "set(${name}_LIBRARIES ${ARGN})\n"
    "set(${name}_INCLUDE_DIR ${incdirs})\n"
    "set(${name}_INCLUDE_DIRS ${incdirs})\n"
  )
 endfunction()
