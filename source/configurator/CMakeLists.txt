project(RoR_Configurator)

set(config_source source/configurator.cpp source/statpict.cpp source/treelistctrl.cpp source/joywizard.cpp)
set(config_headers source/ImprovedConfigFile.h source/statpict.h source/treelistctrl.h source/joysticks.h source/joywizard.h source/utils.h)
set(config_res source/config.xpm source/icon.rc source/joycfg.xpm source/keyboardcfg.xpm source/mousecfg.xpm source/resource.h)

SET(wxWidgets_USE_UNICODE ON)

IF(WIN32)
 SET(wxWidgets_USE_STATIC ON)
ENDIF(WIN32)

FIND_PACKAGE(wxWidgets COMPONENTS base core html net adv)
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
  include_directories (${OGRE_INCLUDE_DIR} ${OGRE_INCLUDE_DEPENDENCIES_DIR})
  include_directories (${OIS_INCLUDE_DIR})
  include_directories (${ROR_MAIN_SOURCE_DIR})

  SET(config_source ${config_source} ${ROR_MAIN_SOURCE_DIR}/InputEngine.cpp)

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
    SET_TARGET_PROPERTIES(rorconfig PROPERTIES COMPILE_FLAGS "/bigobj /MP")
    SET_TARGET_PROPERTIES(rorconfig PROPERTIES LINK_FLAGS    "/SUBSYSTEM:WINDOWS")
  ENDIF(WIN32)


ELSE(wxWidgets_FOUND)
    MESSAGE("wxWidgets not found. Will not build the configurator.")
ENDIF(wxWidgets_FOUND)
