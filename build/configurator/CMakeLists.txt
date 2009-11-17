project(RoR_Configurator)

set(config_source source/configurator.cpp source/statpict.cpp source/treelistctrl.cpp)
set(config_headers source/ImprovedConfigFile.h source/statpict.h source/treelistctrl.h source/joysticks.h)
set(config_res source/config.xpm source/icon.rc source/joycfg.xpm source/keyboardcfg.xpm source/mousecfg.xpm source/resource.h)

SET(wxWidgets_USE_UNICODE ON)

IF(WIN32)
 SET(wxWidgets_USE_STATIC ON)
ENDIF(WIN32)

FIND_PACKAGE(wxWidgets COMPONENTS base core html net)
IF(wxWidgets_FOUND)
  include_directories(${wxWidgets_INCLUDE_DIRS})
  add_definitions(${wxWidgets_DEFINITIONS} ${wxWidgets_CXX_FLAGS})

  IF(WIN32)
    include_directories(${wxWidgets_INCLUDE_DIRS}/msvc)
    add_definitions("-D__WIN32__")
  ENDIF(WIN32)
  IF(UNIX)
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
    INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
  ENDIF(UNIX)
  link_directories (${wxWidgets_LIB_DIR})
  include_directories (${RoR_Dependencies_SOURCE_DIR}/rorserver-sf/source)
  include_directories (${OGRE_INCLUDE_DIR} ${OGRE_INCLUDE_DEPENDENCIES_DIR})
  include_directories (${RoR_Dependencies_SOURCE_DIR}/OIS-HEAD/ois/includes)
  include_directories (${RoR_Main_SOURCE_DIR}/source)

  SET(config_source ${config_source} ${RoR_SOURCE_DIR}/main/source/InputEngine.cpp)

  IF(WIN32)
      source_group(Resources FILES ${config_res})
  ENDIF(WIN32)


  add_definitions("-DNOOGRE -D_UNICODE")

  # always use no debug, otherwise we cannot link against the wxWidgets lib (that used no debug mode on compile time)
  add_definitions("-DNDEBUG")

  #add_definitions("${CMAKE_WXWINDOWS_CXX_FLAGS}")


  add_executable(rorconfig ${config_source} ${config_headers} ${config_res})
  TARGET_LINK_LIBRARIES(rorconfig ${wxWidgets_LIBRARIES} )
  target_link_libraries(rorconfig OgreMain ois)

  IF(WIN32)
    SET_TARGET_PROPERTIES(rorconfig PROPERTIES COMPILE_FLAGS "/bigobj /MT /MP")
    SET_TARGET_PROPERTIES(rorconfig PROPERTIES LINK_FLAGS    "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:\"msvcprt.lib\" /NODEFAULTLIB:\"MSVCRT.lib\"")  
  ENDIF(WIN32)

  install(TARGETS rorconfig
          RUNTIME DESTINATION ${BIN_DIR}
          LIBRARY DESTINATION ${BIN_DIR}
          ARCHIVE DESTINATION ${LIB_DIR})
            
  # already in correct path, no need to install at all!
  #install(TARGETS rorconfig DESTINATION ${BIN_DIR})

ELSE(wxWidgets_FOUND)
    MESSAGE("wxWidgets not found. Will not build the configurator.")
ENDIF(wxWidgets_FOUND)
