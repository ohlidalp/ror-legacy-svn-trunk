project(RoR_Main)
aux_source_directory(source mygui_layouteditor_src)
aux_source_directory(source/Base mygui_layouteditor_src)
aux_source_directory(source/PanelView mygui_layouteditor_src)
include_directories (source source/Base source/PanelView)


include_directories (OgreMain/include Dependencies/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/OIS-HEAD/ois/includes)
include_directories (${RoR_Dependencies_SOURCE_DIR}/mygui/include)

add_executable(layout_editor ${mygui_layouteditor_src} source/LayoutEditor.rc)
IF(WIN32)
SET_TARGET_PROPERTIES(layout_editor PROPERTIES COMPILE_FLAGS "")
SET_TARGET_PROPERTIES(layout_editor PROPERTIES LINK_FLAGS    "/SUBSYSTEM:WINDOWS")  
ENDIF(WIN32)

# add dependencies
add_dependencies(layout_editor OgreMain mygui)

# always required common libs for self generated targets
target_link_libraries(layout_editor
  OgreMain
  ois
  mygui
)

install(TARGETS layout_editor
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
          