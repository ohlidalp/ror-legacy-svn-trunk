project(CaelumLab)

include_directories (../main/include include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/Samples/Common/CEGUIRenderer/include/)
include_directories (/usr/include/CEGUI)
include_directories (/usr/include/OIS)

add_executable(caelumlab "src/CaelumLab.cpp")

# libraries
IF(WIN32)
ELSEIF(UNIX)
  target_link_libraries(caelumlab
    caelum_new
    OgreMain
    CEGUIOgreRenderer
    /usr/lib/libOIS-1.2.0.so
    /usr/lib/libzzip.so
    /usr/lib/libfreetype.so
    /usr/lib/libX11.so
    /usr/lib/libXt.so
    /usr/lib/libgdk-x11-2.0.so
    /usr/lib/libgtk-x11-2.0.so
    /usr/lib/libCEGUIBase.so
    /usr/lib/libXaw.so
  )
ELSEIF(APPLE)
ENDIF(WIN32)

install(TARGETS caelumlab
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
      