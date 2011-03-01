project(RoR_Main)

add_subdirectory(main_sim)

IF(ROR_BUILD_VIEWER)
	add_subdirectory(main_viewer)
ENDIF(ROR_BUILD_VIEWER)
