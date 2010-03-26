project(RoR_Main)

include (RoR.list)

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

include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})

if(ROR_USE_MYGUI)
 include_directories(${MyGUI_INCLUDE_DIRS})
 link_directories   (${MyGUI_LIBRARY_DIRS})
 add_definitions("-DUSE_MYGUI")
endif(ROR_USE_MYGUI)

if(ROR_USE_OPENAL)
 include_directories(${OPENAL_INCLUDE_DIRS})
link_directories   (${OPENAL_LIBRARY_DIRS})
 add_definitions("-DUSE_OPENAL")
endif(ROR_USE_OPENAL) 

if(ROR_USE_LUA)
 include_directories(${LUA_INCLUDE_DIRS})
 link_directories   (${LUA_LIBRARY_DIRS})
 add_definitions("-DUSE_LUA")
endif(ROR_USE_LUA)

if(ROR_USE_SOCKETW)
 include_directories(${SOCKETW_INCLUDE_DIRS})
 link_directories   (${SOCKETW_LIBRARY_DIRS})
 add_definitions("-DUSE_SOCKETW")
endif(ROR_USE_SOCKETW) 

if(ROR_USE_MOFILEREADER)
 include_directories(${MOFILEREADER_INCLUDE_DIRS})
 link_directories   (${MOFILEREADER_LIBRARY_DIRS})
 add_definitions("-DUSE_MOFILEREADER")
endif(ROR_USE_MOFILEREADER)

if(ROR_USE_PAGED)
 include_directories(${PAGED_INCLUDE_DIRS})
 link_directories   (${PAGED_LIBRARY_DIRS})
 add_definitions("-DUSE_PAGED")
endif(ROR_USE_PAGED)

if(ROR_USE_CAELUM)
 include_directories(${CAELUM_INCLUDE_DIRS})
 link_directories   (${CAELUM_LIBRARY_DIRS})
 add_definitions("-DUSE_CAELUM")
endif(ROR_USE_CAELUM)

if(ROR_USE_OIS_G27)
 add_definitions("-DUSE_OIS_G27")
endif(ROR_USE_OIS_G27)

add_executable(RoR ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(RoR ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${MyGUI_LIBRARIES} ${OPENAL_LIBRARIES} ${LUA_LIBRARIES} ${SOCKETW_LIBRARIES} ${MOFILEREADER_LIBRARIES} ${PAGED_LIBRARIES} ${CAELUM_LIBRARIES})
windows_hacks(RoR)

