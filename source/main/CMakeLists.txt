project(RoR_Main)

add_subdirectory(main_sim)

IF(ROR_BUILD_TOOLS)
	add_subdirectory(main_viewer)
	add_subdirectory(main_editor)
ENDIF(ROR_BUILD_TOOLS)
