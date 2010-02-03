add_subdirectory(dependencies)

IF(ROR_BUILD_ANGELSCRIPT_COMPILER)
  add_subdirectory(scriptcompiler)
ENDIF(ROR_BUILD_ANGELSCRIPT_COMPILER)


add_subdirectory(main)
IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
ENDIF(ROR_BUILD_CONFIGURATOR)

IF(ROR_BUILD_INSTALLER)
  add_subdirectory(installer)
ENDIF(ROR_BUILD_INSTALLER)