project(RoR_Installer)

set(resource_files download.xpm action.xpm dest.xpm extrapack.xpm finished.xpm licence.xpm mainpack.xpm streams.xpm unknown.xpm welcome.xpm icon.rc)
set(source_files ConfigManager.cpp wizard.cpp wthread.cpp cevent.cpp symlink.cpp wxStrel.cpp utils.cpp threadpool.cpp wsyncdownload.cpp installerlog.cpp SHA1.cpp)
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

IF(WIN32)
set(INSTALLER_PLATFORM "win32" CACHE PATH "Installer chosen platform")
ENDIF(WIN32)
IF(UNIX)
set(INSTALLER_PLATFORM "ubuntu9.10" CACHE PATH "Installer chosen platform")
ENDIF(UNIX)

FIND_PACKAGE(wxWidgets COMPONENTS base core html adv)
IF(wxWidgets_FOUND)
  include_directories(${wxWidgets_INCLUDE_DIRS})
  add_definitions(${wxWidgets_DEFINITIONS} ${wxWidgets_CXX_FLAGS})
  add_definitions("-DINSTALLER_PLATFORM=\"${INSTALLER_PLATFORM}\"")
  #message("the default installer platform is \"${INSTALLER_PLATFORM}\". To change this, edit INSTALLER_PLATFORM value in cmake.")
  include_directories(${Boost_INCLUDE_DIRS})

  IF(WIN32)
    include_directories(${wxWidgets_INCLUDE_DIRS}/msvc)
    include_directories("C:\\wxWidgets-2.9.0-static\\include C:\\wxWidgets-2.9.0-static\\include\\msvc")

    add_definitions("-D__WIN32__ -D_CRT_SECURE_NO_WARNINGS -DPLATFORM=1 -DWIN32_LEAN_AND_MEAN")

    link_directories ("C:\\Program Files\\boost\\boost_1_39\\lib")
	link_directories ("C:\\wxWidgets-2.9.0-static\\lib\\vc_lib")
  ENDIF(WIN32)
  IF(UNIX)
    add_definitions("-DPLATFORM=2")
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(GTK gtk+-2.0 REQUIRED)
    INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
    link_directories (${wxWidgets_LIB_DIR})
  ENDIF(UNIX)
  include_directories(${Boost_INCLUDE_DIRS})


  add_definitions("-DNOOGRE -D_UNICODE")

  # always use no debug, otherwise we cannot link against the wxWidgets lib (that used no debug mode on compile time)
  add_definitions("-DNDEBUG")

  #add_definitions("${CMAKE_WXWINDOWS_CXX_FLAGS}")

  add_executable(installer ${source_files} ${include_files} ${resource_files})
  windows_hacks(installer)
  
  # we dont need to add ${Boost_LIBRARIES} here, could be wrong as well ...

  IF(WIN32)
    SET_TARGET_PROPERTIES(installer PROPERTIES COMPILE_FLAGS "/bigobj /MT /MP")
    SET_TARGET_PROPERTIES(installer PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS /NODEFAULTLIB:\"msvcprt.lib\" /NODEFAULTLIB:\"MSVCRT.lib\"")


	TARGET_LINK_LIBRARIES(installer
wxbase29u.lib
wxmsw29u_core.lib
wxmsw29u_html.lib
wxmsw29u_adv.lib
wxpng.lib
wxtiff.lib
wxjpeg.lib
wxzlib.lib
wxregexu.lib
wxexpat.lib
winmm
comctl32
rpcrt4
wsock32
	)
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES(installer ${Boost_LIBRARIES})
    TARGET_LINK_LIBRARIES(installer ${wxWidgets_LIBRARIES})
  ENDIF(WIN32)

  # already in correct path, no need to install at all!
  #install(TARGETS rorconfig DESTINATION ${BIN_DIR})

ELSE(wxWidgets_FOUND)
    MESSAGE("wxWidgets not found. Will not build the installer.")
ENDIF(wxWidgets_FOUND)


