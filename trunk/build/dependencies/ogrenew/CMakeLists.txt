cmake_minimum_required(VERSION 2.6)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)

# Welcome to the CMake build system for OGRE.
# This is the main file which takes care of dependencies and
# setting up the general build settings.
project(OGRE3D CXX C)

#Yes, this is duplicated from OgrePrerequisites.h
set(OGRE_VERSION_MAJOR "1")
set(OGRE_VERSION_MINOR "4")
set(OGRE_VERSION_PATCH "9")
set(OGRE_VERSION "${OGRE_VERSION_MAJOR}.${OGRE_VERSION_MINOR}.${OGRE_VERSION_PATCH}")

# Configure target output directories
set(CMAKE_MODULE_PATH "${OGRE3D_SOURCE_DIR}/CMakeModules/")
include(MacroLogFeature)
include(Packaging)

# Create debug libraries with _d postfix
set(CMAKE_DEBUG_POSTFIX "_d")

include(CopyFile)

# Set compiler specific build flags
if (CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-msse)
endif ()
if (MSVC)
  add_definitions(/GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /arch:SSE2 /GF /Gy -D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /SUBSYSTEM:WINDOWS /OPT:NOWIN98")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:CONSOLE /OPT:NOWIN98")
endif (MSVC)

# Add OgreMain include path
include_directories("${OGRE3D_SOURCE_DIR}/OgreMain/include")

# Find dependencies
find_package(Dependencies)

#macro_display_feature_log()

# Provide options for selective builds
option(OGRE_STATIC "Static build (keep LGPL restrictions in mind!)" FALSE)
if (WIN32)
  option(OGRE_BUILD_RENDERSYSTEM_D3D9 "Build Direct3D9 RenderSystem" TRUE)
endif (WIN32)
option(OGRE_BUILD_RENDERSYSTEM_GL "Build OpenGL RenderSystem" TRUE)
option(OGRE_BUILD_PLUGIN_BSP "Build BSP SceneManager plugin" TRUE)
option(OGRE_BUILD_PLUGIN_OCTREE "Build Octree SceneManager plugin" TRUE)
#option(OGRE_BUILD_PLUGIN_PCZ "Build PCZ SceneManager plugin" TRUE)
option(OGRE_BUILD_PLUGIN_PFX "Build ParticleFX plugin" TRUE)
option(OGRE_BUILD_PLUGIN_CG "Build Cg plugin" TRUE)
option(OGRE_BUILD_CEGUIRENDERER "Build CEGUI renderer" TRUE)
option(OGRE_BUILD_SAMPLES "Build Ogre demos" FALSE)
# Provide options for configuring OGRE
option(OGRE_CONFIG_PROFILING "Enable Ogre profiling code" FALSE)
set(OGRE_CONFIG_ASSERTMODE 0 CACHE STRING 
"Set Ogre assertion mode. Possible values:
  0 - Regular asserts in debug, none in release
  1 - Regular asserts in debug, exceptions in release
  2 - Exceptions in debug and release"
)
option(OGRE_CONFIG_DOUBLE "Use doubles instead of floats in Ogre" FALSE)
option(OGRE_CONFIG_MEMTRACK_DEBUG "Enable Ogre's memory tracker in debug mode" FALSE)
option(OGRE_CONFIG_MEMTRACK_RELEASE "Enable Ogre's memory tracker in release mode" FALSE)
set(OGRE_CONFIG_THREADS 0 CACHE STRING 
"Enable Ogre thread support for background loading. Possible values:
  0 - Threading off.
  1 - Full background loading.
  2 - Background resource preparation."
)
option(OGRE_CONFIG_DISABLE_FREEIMAGE "Don't build FreeImage codec. If you use this option, you need to provide your own image handling codecs." TRUE)
option(OGRE_CONFIG_DISABLE_DDS "Don't build DDS codec.")
option(OGRE_CONFIG_NEW_COMPILERS "Use the new script compilers." TRUE)

if (OGRE_STATIC)
  set(OGRE_LIB_TYPE STATIC)
else ()
  set(OGRE_LIB_TYPE SHARED)
endif ()
if (OGRE_CONFIG_THREADS)
  if (NOT BOOST_FOUND)
    message(SEND_ERROR "Could not find dependency: boost-thread. Can't enable threading")
    set(OGRE_CONFIG_THREADS 0)
  else()
    if (UNIX)
      add_definitions(-pthread -lpthread)
    endif()
  endif()
endif()

# Configure OgreConfig.h file
add_subdirectory(CMakeTemplates)

# Setup OgreMain project
add_subdirectory(OgreMain)

# Setup RenderSystems
add_subdirectory(RenderSystems)

# Setup Plugins
add_subdirectory(PlugIns)

# Setup CEGUI renderer
if (OGRE_BUILD_CEGUIRENDERER)
  if (NOT CEGUI_FOUND)
    message(STATUS "Could not find dependency: CEGUI")
    message(STATUS "Skipping CEGUI renderer build")
  else ()
    add_subdirectory(Samples/Common/CEGUIRenderer)
  endif ()
endif ()

# Setup samples
if (OGRE_BUILD_SAMPLES)
  add_subdirectory(Samples) #TODO maybe use optional_add_subdirectory()
endif ()

