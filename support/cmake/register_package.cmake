set(localModuleDir ${PROJECT_BINARY_DIR}/pkg)

function(sgct_register_package name)
    set(incdirs "")
    foreach(target ${ARGN})
        get_target_property(dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
        set(incdirs "${incdirs} ${dirs}")
    endforeach()
    file(WRITE "${localModuleDir}/Find${name}.cmake" 
         "# Fake Find file for ${name}\n"
         "set(${name}_FOUND ON)\n"
         "set(${name}_LIBRARIES ${ARGN})\n"
         "set(${name}_INCLUDE_DIRS ${incdirs})\n"
         )
 endfunction()