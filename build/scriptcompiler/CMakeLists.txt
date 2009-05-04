project(RoR_ScriptCompiler)

FILE(GLOB_RECURSE sc_src  *.cpp *.c *.h)

IF(WIN32)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  set(sc_src ${sc_src} icon.rc)
ELSEIF(UNIX)
ELSEIF(APPLE)
ENDIF(WIN32)

include_directories (interfaces)

include_directories (${RoR_Dependencies_SOURCE_DIR}/angelscript/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/angelscript/add_on)

# main target
add_executable(as_compiler ${sc_src})

# add dependencies
add_dependencies(as_compiler angelscript)
target_link_libraries(as_compiler angelscript)

install(TARGETS as_compiler
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})

  