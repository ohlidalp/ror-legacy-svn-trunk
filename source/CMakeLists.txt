if(NOT ROR_BINTOOL_DIR)
 message(FATAL_ERROR "please use the main directory when using cmake, not only the source directory.")
endif(NOT ROR_BINTOOL_DIR)

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