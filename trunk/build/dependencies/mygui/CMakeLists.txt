#FILE(GLOB_RECURSE mygui_src  src/*.cpp)
FILE(GLOB_RECURSE mygui_headers  include/*.h)

set(mygui_src
  src/MyGUI_Button.cpp
  src/MyGUI_Canvas.cpp
  src/MyGUI_ComboBox.cpp
  src/MyGUI_DDContainer.cpp
  src/MyGUI_Edit.cpp
  src/MyGUI_HScroll.cpp
  src/MyGUI_ItemBox.cpp
  src/MyGUI_List.cpp
  src/MyGUI_ListBox.cpp
  src/MyGUI_ListCtrl.cpp
  src/MyGUI_MenuBar.cpp
  src/MyGUI_MenuCtrl.cpp
  src/MyGUI_MenuItem.cpp
  src/MyGUI_Message.cpp
  src/MyGUI_MultiList.cpp
  src/MyGUI_PopupMenu.cpp
  src/MyGUI_Progress.cpp
  src/MyGUI_ScrollView.cpp
  src/MyGUI_StaticImage.cpp
  src/MyGUI_StaticText.cpp
  src/MyGUI_Tab.cpp
  src/MyGUI_TabItem.cpp
  src/MyGUI_VScroll.cpp
  src/MyGUI_Widget.cpp
  src/MyGUI_Window.cpp
  src/MyGUI_EditText.cpp
  src/MyGUI_MainSkin.cpp
  src/MyGUI_RawRect.cpp
  src/MyGUI_RotatingSkin.cpp
  src/MyGUI_SimpleText.cpp
  src/MyGUI_SubSkin.cpp
  src/MyGUI_TileRect.cpp
  src/MyGUI_LayerItem.cpp
  src/MyGUI_LayerNode.cpp
  src/MyGUI_OverlappedLayer.cpp
  src/MyGUI_RenderItem.cpp
  src/MyGUI_SharedLayer.cpp
  src/MyGUI_SharedLayerNode.cpp
  src/MyGUI_ActionController.cpp
  src/MyGUI_ControllerEdgeHide.cpp
  src/MyGUI_ControllerFadeAlpha.cpp
  src/MyGUI_ControllerPosition.cpp
  src/MyGUI_Exception.cpp
  src/MyGUI_Precompiled.cpp
  src/MyGUI_IWidgetCreator.cpp
  src/MyGUI_ScrollViewBase.cpp
  src/MyGUI_ResourceImageSet.cpp
  src/MyGUI_ResourceImageSetPointer.cpp
  src/MyGUI_ResourceManualFont.cpp
  src/MyGUI_ResourceManualPointer.cpp
  src/MyGUI_ResourceSkin.cpp
  src/MyGUI_ResourceTrueTypeFont.cpp
  src/MyGUI_MaskPickInfo.cpp
  src/MyGUI_Any.cpp
  src/MyGUI_Colour.cpp
  src/MyGUI_ClipboardManager.cpp
  src/MyGUI_ControllerManager.cpp
  src/MyGUI_DataManager.cpp
  src/MyGUI_DelegateManager.cpp
  src/MyGUI_DynLibManager.cpp
  src/MyGUI_FactoryManager.cpp
  src/MyGUI_FontManager.cpp
  src/MyGUI_Gui.cpp
  src/MyGUI_InputManager.cpp
  src/MyGUI_LanguageManager.cpp
  src/MyGUI_LayerManager.cpp
  src/MyGUI_LayoutManager.cpp
  src/MyGUI_PluginManager.cpp
  src/MyGUI_PointerManager.cpp
  src/MyGUI_RenderManager.cpp
  src/MyGUI_ResourceManager.cpp
  src/MyGUI_SkinManager.cpp
  src/MyGUI_SubWidgetManager.cpp
  src/MyGUI_WidgetManager.cpp
  src/MyGUI_DataFileStream.cpp
  src/MyGUI_DataStream.cpp
  src/MyGUI_DynLib.cpp
  src/MyGUI_Guid.cpp
  src/MyGUI_RenderOut.cpp
  src/MyGUI_TextIterator.cpp
  src/MyGUI_TextureUtility.cpp
  src/MyGUI_Timer.cpp
  src/MyGUI_XmlDocument.cpp
  src/MyGUI_LogManager.cpp
  src/MyGUI_LogStream.cpp
  src/MyGUI_DocArray.cpp
  src/MyGUI_DocType.cpp

  src/MyGUI_DataFileStream.cpp
  src/MyGUI_DataStream.cpp
  src/MyGUI_DynLib.cpp
  src/MyGUI_Guid.cpp
  src/MyGUI_RenderOut.cpp
  src/MyGUI_TextIterator.cpp
  src/MyGUI_TextureUtility.cpp
  src/MyGUI_Timer.cpp
  src/MyGUI_XmlDocument.cpp
  

  src/MyGUI_OgreDataManager.cpp
  src/MyGUI_OgreDataStream.cpp
  src/MyGUI_OgreRenderManager.cpp
  src/MyGUI_OgreRTTexture.cpp
  src/MyGUI_OgreTexture.cpp
  src/MyGUI_OgreVertexBuffer.cpp
  src/MyGUI_OverlappedLayer.cpp

  )

include_directories (include)

include_directories (${OGRE_INCLUDE_DIR})
include_directories (${OGRE_INCLUDE_DEPENDENCIES_DIR}) # for freetype
include_directories (${RoR_Dependencies_SOURCE_DIR}/OIS-HEAD/ois/includes)

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DMYGUI_BUILD -DMYGUI_USE_FREETYPE")


# the final lib
add_library(mygui SHARED ${mygui_src} ${mygui_headers})

IF(WIN32)
  SET_TARGET_PROPERTIES(mygui PROPERTIES COMPILE_FLAGS "/MP")
ENDIF(WIN32)

# this is for windows only, since windows is stupid ...
add_dependencies(mygui OgreMain ois)
target_link_libraries(mygui OgreMain ois)

IF(WIN32)
ELSEIF(UNIX)
	target_link_libraries(mygui uuid)
ELSEIF(APPLE)
ENDIF(WIN32)


install(TARGETS mygui
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
      
