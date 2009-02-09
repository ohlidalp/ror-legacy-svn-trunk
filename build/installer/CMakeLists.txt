project(RoR_Installer)


SET(wxWidgets_USE_UNICODE ON)
SET(wxWidgets_USE_STATIC ON)

FIND_PACKAGE(wxWidgets COMPONENTS base core adv html xml)
IF(wxWidgets_FOUND)
  include_directories(${wxWidgets_INCLUDE_DIRS})
  include_directories (${RoR_SOURCE_DIR}/wsync/include)

  link_directories (${wxWidgets_LIB_DIR})
  
  add_definitions("-DNOOGRE -D_UNICODE")
  # always use no debug, otherwise we cannot link against the wxWidgets lib (that used no debug mode on compile time)
  add_definitions("-DNDEBUG")


  set(resource_files download.xpm action.xpm dest.xpm extrapack.xpm finished.xpm licence.xpm mainpack.xpm streams.xpm unknown.xpm welcome.xpm icon.rc)
  set(source_files ConfigManager.cpp wizard.cpp)
  set(include_files ConfigManager.h wizard.h wxStrel.h)
  
    IF(WIN32)
      set(WSYNC_BOOST_DIR "C:\\Program Files\\boost\\boost_1_36_0" CACHE STRING "set the path to your boost directory")
      include_directories(${WSYNC_BOOST_DIR})
      add_definitions("-DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINDOWS -D_WIN32 -D__WXMSW__ -DNOPCH -D__WXDEBUG__ /bigobj")
      link_directories(${WSYNC_BOOST_DIR}/lib)
      source_group(Resources FILES ${resource_files})
    ENDIF(WIN32)

  add_executable(installer  ${resource_files} ${source_files} ${include_files})
  TARGET_LINK_LIBRARIES(installer ${wxWidgets_LIBRARIES} libwsync)

  install(TARGETS installer
          RUNTIME DESTINATION ${BIN_DIR}
          LIBRARY DESTINATION ${BIN_DIR}
          ARCHIVE DESTINATION ${LIB_DIR})
  IF(WIN32)
    SET_TARGET_PROPERTIES(installer PROPERTIES COMPILE_FLAGS "/Zc:wchar_t-")
    SET_TARGET_PROPERTIES(installer PROPERTIES LINK_FLAGS    "/SUBSYSTEM:WINDOWS")  
  ENDIF(WIN32)

ELSE(wxWidgets_FOUND)
    MESSAGE("wxWidgets not found. Will not build the configurator.")
ENDIF(wxWidgets_FOUND)
