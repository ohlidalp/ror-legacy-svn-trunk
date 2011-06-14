add_subdirectory(main)


if(ROR_USE_ANGELSCRIPT)
	add_subdirectory(angelscript_addons)
endif()

set(ROR_BUILD_CONFIGURATOR "TRUE" CACHE BOOL "build the configurator")

IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
  #add_subdirectory(modmanager)
ENDIF(ROR_BUILD_CONFIGURATOR)

IF(ROR_BUILD_UPDATER)
  add_subdirectory(updater)
ENDIF(ROR_BUILD_UPDATER)
