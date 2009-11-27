project(RoR_Installer)

set(resource_files download.xpm action.xpm dest.xpm extrapack.xpm finished.xpm licence.xpm mainpack.xpm streams.xpm unknown.xpm welcome.xpm icon.rc)
set(source_files ConfigManager.cpp wizard.cpp wthread.cpp cevent.cpp symlink.cpp wxStrel.cpp utils.cpp)
set(include_files ConfigManager.h wizard.h wxStrel.h wthread.h cevent.h symlink.h utils.h)

SET(wxWidgets_USE_UNICODE ON)
#SET(wxWidgets_USE_STATIC ON)

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_MULTITHREADED ON)
find_package( Boost 1.3 COMPONENTS date_time filesystem system regex )

if(Boost_FOUND)
else()
  message(" !!! boost not found!")
endif()


FIND_PACKAGE(wxWidgets COMPONENTS base core html adv)
IF(wxWidgets_FOUND)
  include_directories(${wxWidgets_INCLUDE_DIRS})
  add_definitions(${wxWidgets_DEFINITIONS} ${wxWidgets_CXX_FLAGS})
  include_directories(${Boost_INCLUDE_DIRS})

  IF(WIN32)
    include_directories(${wxWidgets_INCLUDE_DIRS}/msvc)
    add_definitions("-D__WIN32__ -D_CRT_SECURE_NO_WARNINGS -DPLATFORM=1")
    link_directories ("C:\\Program Files\\boost\\boost_1_39\\lib")
  ENDIF(WIN32)
  IF(UNIX)
    add_definitions("-DPLATFORM=2")
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
    INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
  ENDIF(UNIX)
  link_directories (${wxWidgets_LIB_DIR})
  include_directories (${RoR_SOURCE_DIR}/wsync/include)
  include_directories(${Boost_INCLUDE_DIRS})


  add_definitions("-DNOOGRE -D_UNICODE")

  # always use no debug, otherwise we cannot link against the wxWidgets lib (that used no debug mode on compile time)
  add_definitions("-DNDEBUG")

  #add_definitions("${CMAKE_WXWINDOWS_CXX_FLAGS}")

  add_executable(installer ${source_files} ${include_files} ${resource_files})
  # we dont need to add ${Boost_LIBRARIES} here, could be wrong as well ...
  target_link_libraries(installer libwsync-static)
  TARGET_LINK_LIBRARIES(installer ${wxWidgets_LIBRARIES} ${Boost_LIBRARIES})

  IF(WIN32)
    SET_TARGET_PROPERTIES(installer PROPERTIES COMPILE_FLAGS "/bigobj /MT /MP")
    SET_TARGET_PROPERTIES(installer PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS /NODEFAULTLIB:\"msvcprt.lib\" /NODEFAULTLIB:\"MSVCRT.lib\"")
  ENDIF(WIN32)

  install(TARGETS installer
          RUNTIME DESTINATION ${BIN_DIR}
          LIBRARY DESTINATION ${BIN_DIR}
          ARCHIVE DESTINATION ${LIB_DIR})

  # already in correct path, no need to install at all!
  #install(TARGETS rorconfig DESTINATION ${BIN_DIR})

ELSE(wxWidgets_FOUND)
    MESSAGE("wxWidgets not found. Will not build the installer.")
ENDIF(wxWidgets_FOUND)


