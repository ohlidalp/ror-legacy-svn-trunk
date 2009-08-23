project(RoR_Dependencies)

# non - ogre dependent
if(ROR_BUILD_LUASCRIPT)
 add_subdirectory("lua-5.1.2")
endif(ROR_BUILD_LUASCRIPT)

add_subdirectory("mysocketw")
add_subdirectory("OIS-HEAD")
#add_subdirectory("openal-soft-1.5.304")
#add_subdirectory("openal-soft-1.6.372")
add_subdirectory("openal-soft-1.8.466")

set(OGRE_SET_DISABLE_FREEIMAGE 1)
add_subdirectory("ogrenew")

# OGRE dependent
add_subdirectory("Caelum-2006-11-09")

if(ROR_BUILD_HYDRAX)
  add_subdirectory("hydrax")
endif(ROR_BUILD_HYDRAX)

if(ROR_BUILD_PAGED)
if(ROR_BUILD_PAGED_SF)
  add_subdirectory("pagedgeometry_sf")
else(ROR_BUILD_PAGED_SF)
  add_subdirectory("PagedGeometry")
endif(ROR_BUILD_PAGED_SF)
endif(ROR_BUILD_PAGED)

IF(ROR_BUILD_ANGELSCRIPT)
  add_subdirectory(angelscript)
ENDIF(ROR_BUILD_ANGELSCRIPT)

add_subdirectory("moFileReader")

add_subdirectory("mygui")
