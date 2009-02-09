CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )

project(RoR)

# setup path for cmake add-ons
SET(RoR_Dependencies_CMAKE_DIR "${RoR_SOURCE_DIR}/dependencies/cmake")
SET(BUILD_SHARED_LIBS ON)

IF(WIN32)
  SET(PLATFORM "windows")
  #add_definitions("-DWIN32 -D_WINDOWS -D__WIN32__")
ELSEIF(UNIX)
  SET(PLATFORM "linux")
  add_definitions("-DNWINDOWS")
ELSEIF(APPLE)
  SET(PLATFORM "macos")
  add_definitions("-DNWINDOWS")
ENDIF(WIN32)

# build types
IF (CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions("-D_DEBUG")
  SET(BUILD_DIR "debug")
ELSEIF (CMAKE_BUILD_TYPE MATCHES "Release")
  add_definitions("-DNDEBUG")
  SET(BUILD_DIR "release")
ELSEIF (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
  #add_definitions("-DDEBUG")
  # override bin dir here, as we want to use the same as release
  SET(BUILD_DIR "release")
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

# set the make install target
SET(BIN_DIR ${RoR_SOURCE_DIR}/bin/${BUILD_DIR}/${PLATFORM})
SET(LIB_DIR ${RoR_SOURCE_DIR}/build/lib)
SET(LIBRARY_OUTPUT_DIRECTORY ${RoR_SOURCE_DIR}/build/lib)

# the changed directories break the windows build, so leave it as default
#SET(EXECUTABLE_OUTPUT_PATH   ${BIN_DIR})
#SET(LIBRARY_OUTPUT_PATH      ${BIN_DIR})
#SET(LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})
#SET(ARCHIVE_OUTPUT_DIRECTORY ${BIN_DIR})
#SET(RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# RoR options
option(ROR_BUILD_HYDRAX "build RoR with HYDRAX water engine support" TRUE)
option(ROR_BUILD_PAGED "build RoR with Paged Geometry support (grass and trees)" TRUE)
option(ROR_BUILD_BEAMSTATS "build RoR extensive Beam engine debugging/profiling support (do not enable in release build)" FALSE)
option(ROR_BUILD_LUASCRIPT "adds the LUA Scripting engine" TRUE)
option(ROR_BUILD_CONFIGURATOR "builds the RoRConfig" TRUE)
option(ROR_BUILD_ROR "toggles building of RoR and its dependencies" TRUE)
option(WSYNC_BUILD_WSYNC "builds the wsync tool" TRUE)

#experimental ones
option(ROR_BUILD_PYTHONSCRIPT "adds the Python Scripting engine" FALSE)
option(WSYNC_BUILD_INSTALLER "builds the installer" TRUE)
option(ROR_BUILD_NEWNET "builds the new network approach using raknet" FALSE)
option(ROR_BUILD_CLOUDS "build RoR with clouds engine support (windows only, experimental)" FALSE)
option(CAELUM_BUILD_NEW "uses a more recent caelum version" FALSE)
option(GUI_BUILD_LAYOUT_EDITOR "builds the GUI LAYOUT EDITOR" TRUE)

IF(WIN32)
  option(ROR_BUILD_XFIRE "builds the XFire Extensions" FALSE)
  option(ROR_BUILD_BUGTRAP "build RoR with Bugtrap support (windows only)" FALSE)
ELSE(WIN32)
  set(ROR_BUILD_BUGTRAP FALSE)
  set(ROR_BUILD_XFIRE FALSE)
ENDIF(WIN32)



# unused flags:
# OIS_HEAD (link against old or svn head version of OIS)
# HAS_EDITOR (include ingame truck editor [the old one])
# OGREPLUGIN (use calls to terrain manager in order to update terrain visuals (obsolete) )

IF(ROR_BUILD_ROR)
  add_subdirectory(dependencies)
  add_subdirectory(main)
  IF(ROR_BUILD_CONFIGURATOR)
    add_subdirectory(configurator)
  ENDIF(ROR_BUILD_CONFIGURATOR)

  IF(GUI_BUILD_LAYOUT_EDITOR)
    add_subdirectory(LayoutEditor)
  ENDIF(GUI_BUILD_LAYOUT_EDITOR)
ENDIF(ROR_BUILD_ROR)

IF(WSYNC_BUILD_WSYNC)
  add_subdirectory(wsync)
ENDIF(WSYNC_BUILD_WSYNC)

IF(WSYNC_BUILD_INSTALLER)
  add_subdirectory(installer)
ENDIF(WSYNC_BUILD_INSTALLER)

if(ROR_BUILD_NEWNET)
  add_subdirectory(rorserver_new)
endif(ROR_BUILD_NEWNET)

message(STATUS "==== RoR Configuration =======")
message(STATUS " * HYDRAX water:          ${ROR_BUILD_HYDRAX}")
message(STATUS " * paged geometry:        ${ROR_BUILD_PAGED}")
message(STATUS " * clouds engine:         ${ROR_BUILD_CLOUDS}")
message(STATUS " * Profiling (Beamstats): ${ROR_BUILD_BEAMSTATS}")
message(STATUS " * Python Script Engine:  ${ROR_BUILD_PYTHONSCRIPT}")
message(STATUS " * LUA Script Engine:     ${ROR_BUILD_LUASCRIPT}")
message(STATUS "==============================")

