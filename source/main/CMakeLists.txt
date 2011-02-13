project(RoR_Main)

#setup libraries
macro(setup_lib name)
   if(ROR_USE_${name})
      include_directories(${${name}_INCLUDE_DIRS})
      link_directories   (${${name}_LIBRARY_DIRS})
      add_definitions("-DUSE_${name}")
      set(optional_libs ${optional_libs} ${${name}_LIBRARIES})
      message(STATUS "${name} Enabled:      \tYES")
      message(STATUS "${name}_INCLUDE_DIRS: \t${${name}_INCLUDE_DIRS}")
      if(${${name}_LIBRARY_DIRS})
         message(STATUS "${name}_LIBRARY_DIRS: \t${${name}_LIBRARY_DIRS}")
      endif(${${name}_LIBRARY_DIRS})
      message(STATUS "${name}_LIBRARIES:    \t${${name}_LIBRARIES}")
   else()
      message(STATUS "${name} Enabled:      \tNO")
   endif(ROR_USE_${name})
endmacro(setup_lib)

macro(add_sub_dir name)
	FILE(GLOB_RECURSE ${name}_source ${name}/*.cpp)
	FILE(GLOB_RECURSE ${name}_header ${name}/*.h)
	SOURCE_GROUP("${name}" FILES ${${name}_source} ${${name}_header})
	include_directories(${name})
	set(sources ${sources} ${${name}_source})
	set(headers ${headers} ${${name}_header})
endmacro(add_sub_dir)

# config below
IF(WIN32)
   include_directories(${BOOST_INCLUDE_DIRS})
   link_directories   (${BOOST_LIBRARY_DIRS})
   include_directories(${PThread_INCLUDE_DIRS})
   link_directories   (${PThread_LIBRARY_DIRS})
   set(OS_LIBS "${PThread_LIBRARIES};dinput8.lib;dxguid.lib")

   # disable some annoying VS warnings:
   # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
   # warning C4305: 'initializing' : truncation from 'double' to 'const float'
   add_definitions("/wd4305 /wd4244 -DNOMINMAX")
ELSEIF(UNIX)
   include_directories(${GTK_INCLUDE_DIRS})
   set(OS_LIBS "X11")
ENDIF(WIN32)

include_directories(${RoR_SOURCE_DIR}/source/protocol)

# required components
include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ogre_Terrain_INCLUDE_DIRS})
link_directories   (${Ogre_Terrain_LIBRARY_DIRS})
include_directories(${Ogre_Paging_INCLUDE_DIRS})
link_directories   (${Ogre_Paging_LIBRARY_DIRS})
include_directories(${Ogre_RTShader_INCLUDE_DIRS})
link_directories   (${Ogre_RTShader_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})

# optional components
setup_lib(MYGUI)
setup_lib(OPENAL)
setup_lib(LUA)
setup_lib(CURL)
setup_lib(SOCKETW)
setup_lib(MOFILEREADER)
setup_lib(PAGED)
setup_lib(CAELUM)
setup_lib(ANGELSCRIPT)
setup_lib(HYDRAX)
setup_lib(CRASHRPT)

if(ROR_USE_ANGELSCRIPT)
	add_definitions("-DAS_USE_NAMESPACE")
	set(optional_libs ${optional_libs} angelscript_addons)
endif()

if(ROR_USE_OIS_G27)
   add_definitions("-DUSE_OIS_G27")
endif(ROR_USE_OIS_G27)

if(ROR_FEAT_TIMING)
   add_definitions("-DFEAT_TIMING")
endif(ROR_FEAT_TIMING)


add_definitions("-DBOOST_ALL_NO_LIB")
add_definitions("-DUSE_RTSHADER_SYSTEM")

include_directories(.)
add_sub_dir(audio)
add_sub_dir(gameplay)
add_sub_dir(gfx)
add_sub_dir(gfx/dof)
add_sub_dir(gui)
add_sub_dir(gui/mygui)
add_sub_dir(physics)
add_sub_dir(physics/air)
add_sub_dir(physics/collision)
add_sub_dir(physics/flex)
add_sub_dir(physics/input_output)
add_sub_dir(physics/mplatform)
add_sub_dir(physics/utils)
add_sub_dir(physics/water)
add_sub_dir(scripting)
add_sub_dir(utils)
add_sub_dir(main_sim)
add_sub_dir(network)
add_sub_dir(network/protocol)

IF(WIN32)
	add_executable(RoR WIN32 ${headers} ${sources} main_sim/icon.rc)
ELSE(WIN32)
	add_executable(RoR ${headers} ${sources})
ENDIF(WIN32)

target_link_libraries(RoR ${Ogre_LIBRARIES} ${Ogre_Terrain_LIBRARIES} ${Ogre_Paging_LIBRARIES} ${Ogre_RTShader_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${optional_libs})

