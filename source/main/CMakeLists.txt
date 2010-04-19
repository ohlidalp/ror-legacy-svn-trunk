project(RoR_Main)

# sources
include (RoR.list)

#setup libraries
macro(setup_lib name)
   if(ROR_USE_${name})
      message(STATUS "Enabled:     ${name}")
      include_directories(${${name}_INCLUDE_DIRS})
      link_directories   (${${name}_LIBRARY_DIRS})
      add_definitions("-DUSE_${name}")
      set(optional_libs ${optional_libs} ${${name}_LIBRARIES})
      message(STATUS "${name}_INCLUDE_DIRS:     ${${name}_INCLUDE_DIRS}")
      message(STATUS "${name}_LIBRARY_DIRS:     ${${name}_LIBRARY_DIRS}")
      message(STATUS "${name}_LIBRARIES:     ${${name}_LIBRARIES}")
   else()
      message(STATUS "Not enabled: ${name}")
   endif(ROR_USE_${name})
endmacro(setup_lib)

# config below
IF(WIN32)
   include_directories(${PThread_INCLUDE_DIRS})
   link_directories   (${PThread_LIBRARY_DIRS})
   set(OS_LIBS "${PThread_LIBRARIES} dinput8.lib dxguid.lib")

   # disable some annoying VS warnings:
   # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
   # warning C4305: 'initializing' : truncation from 'double' to 'const float'
   add_definitions("/wd4305 /wd4244 -DNOMINMAX")
ELSEIF(UNIX)
   include_directories(${GTK_INCLUDE_DIRS})
   set(OS_LIBS "X11")
ENDIF(WIN32)

# required components
include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})

# optional components
setup_lib(MYGUI)
setup_lib(OPENAL)
setup_lib(LUA)
setup_lib(SOCKETW)
setup_lib(MOFILEREADER)
setup_lib(PAGED)
setup_lib(CAELUM)
setup_lib(ANGELSCRIPT)
setup_lib(HYDRAX)
setup_lib(WINDOWS_CRASH_REPORT)

if(ROR_USE_OIS_G27)
   add_definitions("-DUSE_OIS_G27")
endif(ROR_USE_OIS_G27)

add_executable(RoR ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(RoR ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${optional_libs})
windows_hacks(RoR)

