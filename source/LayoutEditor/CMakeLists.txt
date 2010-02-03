project(RoR_LayoutEditor)

FILE(GLOB_RECURSE layout_editor_src  *.cpp *.c *.h)

IF(WIN32)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  set(layout_editor_src ${layout_editor_src} LayoutEditor.rc)
ELSEIF(UNIX)
ELSEIF(APPLE)
ENDIF(WIN32)

include_directories (.)
include_directories (${RoR_Dependencies_SOURCE_DIR}/mygui/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/OIS-HEAD/ois/includes)
include_directories (${OGRE_INCLUDE_DIR})


# main target
add_executable(layout_editor ${layout_editor_src})
IF(WIN32)
  SET_TARGET_PROPERTIES(layout_editor PROPERTIES COMPILE_FLAGS "/MP")
ENDIF(WIN32)

# add dependencies
add_dependencies(layout_editor mygui ois OgreMain)
target_link_libraries(layout_editor mygui ois OgreMain)

install(TARGETS layout_editor
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})

  