FILE(GLOB_RECURSE mygui_src  src/*.cpp)
FILE(GLOB_RECURSE mygui_headers  include/*.h)

include_directories (include)

include_directories (${OGRE_INCLUDE_DIR})
include_directories (${OGRE_INCLUDE_DEPENDENCIES_DIR}) # for freetype
include_directories (${OIS_INCLUDE_DIR})

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DMYGUI_BUILD -DMYGUI_USE_FREETYPE")

# the final lib
add_library(mygui SHARED ${mygui_src} ${mygui_headers})

# this is for windows only, since windows is stupid ...
add_dependencies(mygui OgreMain ois)
target_link_libraries(mygui OgreMain ois)

IF(WIN32)
ELSEIF(UNIX)
	target_link_libraries(mygui uuid)
ELSEIF(APPLE)
ENDIF(WIN32)
