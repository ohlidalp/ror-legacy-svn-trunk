CMAKE_MINIMUM_REQUIRED( VERSION 2.6.0 )

project(RoR)

#detup the default build mode
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# setup path for cmake add-ons
SET(RoR_Dependencies_CMAKE_DIR "${RoR_SOURCE_DIR}/dependencies/cmake")
SET(BUILD_SHARED_LIBS ON)

# setup ogre variables
SET(OGRE_INCLUDE_DIR "${RoR_SOURCE_DIR}/dependencies/ogrenew/OgreMain/include")
SET(OGRE_LIBRARIES "OgreMain")
SET(OGRE_LIB_DIR "${RoR_SOURCE_DIR}/dependencies/ogrenew/lib")
mark_as_advanced(OGRE_INCLUDE_DIR OGRE_LIBRARIES OGRE_LIB_DIR)

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

option(ROR_OGRE_NEWER "use Ogre 1.6.x instead of 1.4.3 (Highly Experimental!)" FALSE)
if(ROR_OGRE_NEWER)
 #ogre 1.6 resides in ogre directory
 SET(OGRE_DIR "ogre")
ELSE(ROR_OGRE_NEWER)
 #ogre 1.4 resides in ogrenew directory
 SET(OGRE_DIR "ogrenew")
ENDIF(ROR_OGRE_NEWER)
SET(OGRE_INCLUDE_DIR ${RoR_SOURCE_DIR}/dependencies/${OGRE_DIR}/OgreMain/include)
SET(OGRE_INCLUDE_DEPENDENCIES_DIR ${RoR_SOURCE_DIR}/dependencies/${OGRE_DIR}/Dependencies/include)


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
option(ROR_BUILD_PAGED_SF "use latest Paged Geometry support from SF" FALSE)

option(ROR_BUILD_CONFIGURATOR "builds the RoRConfig" TRUE)

option(ROR_BUILD_TRAFFIC_SUPPORT "adds AI traffic support to RoR" FALSE)
option(ROR_BUILD_OPENCL_SUPPORT "adds OpenCL support to ROR" FALSE)

# optimization things
option(ROR_BUILD_BEAMSTATS "build RoR extensive Beam engine debugging/profiling support (do not enable in release build)" FALSE)
option(ROR_USE_OPTIMIZATIONS_WINDOWS "optimize build, gives more FPS, but needs longer to compile" TRUE)

#option(ROR_BUILD_ANGELSCRIPT "build and use the new scripting engine" FALSE)
set(ROR_BUILD_ANGELSCRIPT TRUE)
option(ROR_BUILD_ANGELSCRIPT_COMPILER "build the AS compiler tool." FALSE)
option(WSYNC_BUILD_WSYNC "builds the wsync tool" FALSE)
#set(WSYNC_BUILD_WSYNC FALSE)
option(WSYNC_BUILD_INSTALLER "builds the wsync-installer tool" FALSE)
#option(ROR_BUILD_LUASCRIPT "adds the LUA Scripting engine" TRUE)
set(ROR_BUILD_LUASCRIPT TRUE)

IF(WIN32)
  set(ROR_BUILD_XFIRE TRUE)
  #option(ROR_BUILD_BUGTRAP "build RoR with Bugtrap support (windows only)" FALSE)
  set(ROR_BUILD_BUGTRAP FALSE)
  set(NVIDIA_CG_DIR "$ENV{PROGRAMFILES}\\NVIDIA Corporation\\Cg" CACHE PATH "directory of the nvidia cg toolkit installation")
ELSE(WIN32)
  set(ROR_BUILD_BUGTRAP FALSE)
  set(ROR_BUILD_XFIRE FALSE)
  set(NVIDIA_CG_DIR "")
ENDIF(WIN32)

IF(ROR_BUILD_ANGELSCRIPT_COMPILER)
  add_subdirectory(scriptcompiler)
ENDIF(ROR_BUILD_ANGELSCRIPT_COMPILER)


# unused flags:
# OIS_HEAD (link against old or svn head version of OIS)
# HAS_EDITOR (include ingame truck editor [the old one])
# OGREPLUGIN (use calls to terrain manager in order to update terrain visuals (obsolete) )

add_subdirectory(dependencies)
add_subdirectory(main)
IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
ENDIF(ROR_BUILD_CONFIGURATOR)

IF(WSYNC_BUILD_WSYNC)
  add_subdirectory(wsync)
ENDIF(WSYNC_BUILD_WSYNC)

IF(WSYNC_BUILD_INSTALLER)
  if(WSYNC_BUILD_WSYNC)
    add_subdirectory(installer)
  else(WSYNC_BUILD_WSYNC)
    message("to build the installer, you need to build wsync as well. Disabling the installer.")
  endif(WSYNC_BUILD_WSYNC)
ENDIF(WSYNC_BUILD_INSTALLER)
