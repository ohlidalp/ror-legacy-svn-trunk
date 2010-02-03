project(RoR_Dependencies)

# non - ogre dependent
if(ROR_BUILD_LUASCRIPT)
  add_subdirectory("lua-5.1.2")
endif(ROR_BUILD_LUASCRIPT)
IF(ROR_BUILD_ANGELSCRIPT)
  add_subdirectory("angelscript")
ENDIF(ROR_BUILD_ANGELSCRIPT)

add_subdirectory("mysocketw")
add_subdirectory("OIS-HEAD")
add_subdirectory("openal-soft-1.8.466")
add_subdirectory("moFileReader")

# add ogre
# TODO: do not add local sources if usable version found in the operating system
set(OGRE_SET_DISABLE_FREEIMAGE 1)
add_subdirectory(ogre)

# now OGRE dependend packages
add_subdirectory("Caelum-2006-11-09")
add_subdirectory("mygui")

if(ROR_BUILD_HYDRAX)
  add_subdirectory("hydrax")
endif(ROR_BUILD_HYDRAX)

if(ROR_BUILD_PAGED)
  add_subdirectory("PagedGeometry")
endif(ROR_BUILD_PAGED)
