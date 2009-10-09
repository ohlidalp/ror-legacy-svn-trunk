FILE(GLOB_RECURSE mygui_src  src/*.cpp)
FILE(GLOB_RECURSE mygui_headers  include/*.h)

include_directories (include)

include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/Dependencies/include) # for freetype
include_directories (${RoR_Dependencies_SOURCE_DIR}/OIS-HEAD/ois/includes)

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DMYGUI_BUILD")


# the final lib
add_library(mygui SHARED ${mygui_src} ${mygui_headers})

# this is for windows only, since windows is stupid ...
add_dependencies(mygui OgreMain)
target_link_libraries(mygui OgreMain)

IF(WIN32)
ELSEIF(UNIX)
#  target_link_libraries(mygui 
#/usr/lib/libossp-uuid_dce.so
#  )
ELSEIF(APPLE)
ENDIF(WIN32)


install(TARGETS mygui
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
      
