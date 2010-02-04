project(RoR_ScriptCompiler)

FILE(GLOB_RECURSE sc_src  *.cpp *.c *.h)

IF(WIN32)
  set(sc_src ${sc_src} icon.rc)
ENDIF(WIN32)

include_directories (interfaces)
include_directories (${AS_INCLUDE_DIR})
include_directories (${AS_ADDON_DIR})

# main target
add_executable(as_compiler ${sc_src})
windows_hacks(as_compiler)

# add dependencies
add_dependencies(as_compiler angelscript)
target_link_libraries(as_compiler angelscript)
