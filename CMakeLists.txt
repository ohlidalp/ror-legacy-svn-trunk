# ensure cmake features we need
CMAKE_MINIMUM_REQUIRED( VERSION 2.6.0 )
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
# done with cmake check

project(RoR)

#setup the default build mode
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# global cmake options
SET(BUILD_SHARED_LIBS ON)

# setup paths
SET(ROR_DEPENDENCY_DIR       "${RoR_SOURCE_DIR}/source/dependencies")
SET(RUNTIME_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH   ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH      ${RUNTIME_OUTPUT_DIRECTORY})
SET(RoR_Dependencies_CMAKE_DIR "${ROR_DEPENDENCY_DIR}/cmake")

# setup project specific  variables
SET(OGRE_INCLUDE_DIR              ${ROR_DEPENDENCY_DIR}/ogre/OgreMain/include)
SET(OGRE_INCLUDE_DEPENDENCIES_DIR ${ROR_DEPENDENCY_DIR}/ogre/Dependencies/include)
SET(OIS_INCLUDE_DIR               ${ROR_DEPENDENCY_DIR}/OIS-HEAD/ois/includes)
SET(MYGUI_INCLUDE_DIR             ${ROR_DEPENDENCY_DIR}/mygui/include)
SET(AS_INCLUDE_DIR                ${ROR_DEPENDENCY_DIR}/angelscript/include)
SET(AS_ADDON_DIR                  ${ROR_DEPENDENCY_DIR}/angelscript/add_on)
SET(MYSOCKETW_INCLUDE_DIR         ${ROR_DEPENDENCY_DIR}/mysocketw)
SET(CAELUM_INCLUDE_DIR            ${ROR_DEPENDENCY_DIR}/Caelum-2006-11-09/include)
SET(OPENAL_INCLUDE_DIR            ${ROR_DEPENDENCY_DIR}/openal-soft-1.8.466/include ${ROR_DEPENDENCY_DIR}//openal-soft-1.8.466/include/AL)
SET(MOFILEREADER_INCLUDE_DIR      ${ROR_DEPENDENCY_DIR}/moFileReader/include)
SET(HYDRAX_INCLUDE_DIR            ${ROR_DEPENDENCY_DIR}/hydrax)
SET(PAGED_INCLUDE_DIR             ${ROR_DEPENDENCY_DIR}/PagedGeometry/include)
SET(LUA_INCLUDE_DIR               ${ROR_DEPENDENCY_DIR}/lua-5.1.2/src)
SET(WIN32PTHREADS_INCLUDE_DIR     ${ROR_DEPENDENCY_DIR}/pthreads/Pre-built.2/include)
SET(WIN32PTHREADS_LIB             ${ROR_DEPENDENCY_DIR}/pthreads/Pre-built.2/lib/pthreadVC2.lib)
SET(ROR_MAIN_SOURCE_DIR           ${RoR_SOURCE_DIR}/source/main/source)

set(ROR_CMAKE_DEBUG FALSE)

if(ROR_CMAKE_DEBUG)
message("Used Directories:
OGRE_INCLUDE_DIR:              ${OGRE_INCLUDE_DIR}
OGRE_INCLUDE_DEPENDENCIES_DIR: ${OGRE_INCLUDE_DEPENDENCIES_DIR}
OIS_INCLUDE_DIR:               ${OIS_INCLUDE_DIR}
MYGUI_INCLUDE_DIR:             ${MYGUI_INCLUDE_DIR}
AS_INCLUDE_DIR:                ${AS_INCLUDE_DIR}
AS_ADDON_DIR:                  ${AS_ADDON_DIR}
MYSOCKETW_INCLUDE_DIR:         ${MYSOCKETW_INCLUDE_DIR}
CAELUM_INCLUDE_DIR:            ${CAELUM_INCLUDE_DIR}
OPENAL_INCLUDE_DIR:            ${OPENAL_INCLUDE_DIR}
MOFILEREADER_INCLUDE_DIR:      ${MOFILEREADER_INCLUDE_DIR}
HYDRAX_INCLUDE_DIR:            ${HYDRAX_INCLUDE_DIR}
PAGED_INCLUDE_DIR:             ${PAGED_INCLUDE_DIR}
LUA_INCLUDE_DIR:               ${LUA_INCLUDE_DIR}
WIN32PTHREADS_INCLUDE_DIR:     ${WIN32PTHREADS_INCLUDE_DIR}
WIN32PTHREADS_LIB:             ${WIN32PTHREADS_LIB}
ROR_MAIN_SOURCE_DIR:           ${ROR_MAIN_SOURCE_DIR}
")
endif(ROR_CMAKE_DEBUG)

# RoR options
option(ROR_BUILD_HYDRAX               "build RoR with HYDRAX water engine support" TRUE)
option(ROR_BUILD_PAGED                "build RoR with Paged Geometry support (grass and trees)" TRUE)
option(ROR_BUILD_CONFIGURATOR         "builds the RoRConfig" TRUE)
option(ROR_BUILD_BEAMSTATS            "build RoR extensive Beam engine debugging/profiling support (do not enable in release build)" FALSE)
option(ROR_BUILD_ANGELSCRIPT_COMPILER "build the AS compiler tool." FALSE)
option(ROR_BUILD_INSTALLER            "builds the wsync-installer tool" FALSE)

# required options, enabled by default
set(ROR_BUILD_ANGELSCRIPT TRUE)
set(ROR_BUILD_LUASCRIPT TRUE)

# hide some settings
mark_as_advanced(
  CMAKE_DEBUG_POSTFIX
  CMAKE_BACKWARDS_COMPATIBILITY
  Boost_LIB_DIAGNOSTIC_DEFINITIONS
  EXECUTABLE_OUTPUT_PATH
  LIBRARY_OUTPUT_PATH
)

# enforce option dependencies
IF(WIN32)
  set(ROR_BUILD_XFIRE TRUE)
  option(ROR_BUILD_CRASHRPT "build RoR with CrashRPT support (windows only, only use if you produce an official release: complicated)" FALSE)
  #set(ROR_BUILD_CRASHRPT FALSE)
  set(NVIDIA_CG_DIR "$ENV{PROGRAMFILES}\\NVIDIA Corporation\\Cg" CACHE PATH "directory of the nvidia cg toolkit installation")
ELSE(WIN32)
  set(ROR_BUILD_CRASHRPT FALSE)
  set(ROR_BUILD_XFIRE FALSE)
  set(NVIDIA_CG_DIR "")
ENDIF(WIN32)


if(WIN32)
  # add multi processor compilation flags
  set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}        /MP /GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /MP")
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}     /MP")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /Od")
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}          /MP /Od")
  # some general flags
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  include_directories(${DirectX_INCLUDE_DIR})

  set(CMAKE_EXE_LINKER_FLAGS_RELEASE      "${CMAKE_EXE_LINKER_FLAGS_RELEASE}      /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:NOWIN98")
  set(CMAKE_EXE_LINKER_FLAGS_RelWithDebug "${CMAKE_EXE_LINKER_FLAGS_RelWithDebug} /SUBSYSTEM:CONSOLE /OPT:NOWIN98")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE      "${CMAKE_EXE_LINKER_FLAGS_RELEASE}      /GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /MP")
  set(CMAKE_EXE_LINKER_FLAGS_RelWithDebug "${CMAKE_EXE_LINKER_FLAGS_RelWithDebug} /Od")
ENDIF(WIN32)

IF(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE      "${CMAKE_EXE_LINKER_FLAGS_RELEASE}      -mfpmath=sse -msse2 -mmmx -msse -msse3 -m3dnow -O2 -fomit-frame-pointer -fstrict-aliasing -ffast-math -ftracer")
  set(CMAKE_EXE_LINKER_FLAGS_RelWithDebug "${CMAKE_EXE_LINKER_FLAGS_RelWithDebug} -O0")
endif(UNIX)

macro(windows_hacks NAME)
IF(WIN32)
	if (MSVC_IDE)
		# hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
		set_target_properties(${NAME} PROPERTIES PREFIX "../")
		set_target_properties(${NAME} PROPERTIES IMPORT_PREFIX "../")
	endif(MSVC_IDE)
ENDIF(WIN32)
endmacro(windows_hacks)

add_subdirectory(source)

