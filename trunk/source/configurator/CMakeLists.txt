project(RoR_Configurator)

# find wxWidgets
SET(wxWidgets_USE_UNICODE ON)
FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS base core html net adv)
include_directories(${wxWidgets_INCLUDE_DIRS})
add_definitions(${wxWidgets_DEFINITIONS} ${wxWidgets_CXX_FLAGS})
link_directories (${wxWidgets_LIB_DIR})

IF(WIN32)
 include_directories(${BOOST_PATH})
 link_directories   (${BOOST_PATH}/lib)
 include_directories(${PThread_INCLUDE_DIRS})
 link_directories   (${PThread_LIBRARY_DIRS})

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

FILE(GLOB sources "*.cpp")
add_executable(rorconfig ${sources} ${OS_SOURCE} ../main/InputEngine.cpp)
windows_hacks(rorconfig)

TARGET_LINK_LIBRARIES(rorconfig ${wxWidgets_LIBRARIES} ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS})

