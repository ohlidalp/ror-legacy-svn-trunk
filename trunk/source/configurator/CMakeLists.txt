project(RoR_Configurator)

IF(WIN32)
 include_directories(${BOOST_INCLUDE_DIRS})
 link_directories   (${BOOST_LIBRARY_DIRS})
 include_directories(${PThread_INCLUDE_DIRS})
 link_directories   (${PThread_LIBRARY_DIRS})
 include_directories(${WXWIDGETS_SHARED_INCLUDE_DIRS})
 link_directories   (${WXWIDGETS_SHARED_LIBRARY_DIRS})
 
 set(OS_LIBS "${PThread_LIBRARIES} dinput8.lib dxguid.lib")
 set(OS_SOURCE "icon.rc")

 # disable some annoying VS warnings:
 # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
 # warning C4305: 'initializing' : truncation from 'double' to 'const float'
 add_definitions("/wd4305 /wd4244")

ELSEIF(UNIX)
 find_package(PkgConfig)
 PKG_CHECK_MODULES  (GTK gtk+-2.0 REQUIRED)
 INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})

 PKG_CHECK_MODULES  (Ogre OGRE REQUIRED)
 PKG_CHECK_MODULES  (Ois OIS REQUIRED)

 set(OS_SOURCE "")
ENDIF(WIN32)

include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})

add_definitions("-DNOOGRE -D_UNICODE")
include_directories(../main)

if(WIN32)
  # we have to extend the cflags to add USE_CONSOLE definition for non release modes
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}     /D USE_CONSOLE")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /D USE_CONSOLE")
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}          /D USE_CONSOLE")
endif(WIN32)


FILE(GLOB sources "*.cpp")
FILE(GLOB headers "*.h")
add_executable(rorconfig ${sources} ${headers} ${OS_SOURCE} ../main/InputEngine.cpp)
windows_hacks(rorconfig)

TARGET_LINK_LIBRARIES(rorconfig ${WXWIDGETS_SHARED_LIBRARIES} ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS})



