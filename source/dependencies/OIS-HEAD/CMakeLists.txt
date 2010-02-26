aux_source_directory(ois/src/ ois_src)
FILE(GLOB_RECURSE ois_headers  ois/includes/*.h)
include_directories (ois/includes)

# general flags
add_definitions("-DOIS_DYNAMIC_LIB -DOIS_NONCLIENT_BUILD")

SET(EnableFunctionLevelLinking TRUE)
SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "5")
SET(LIB_BUILD_VERSION "0")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_BUILD_VERSION}")
SET(LIB_INSTALL_DIR "lib")

# platform
IF(WIN32)
  include_directories (${DirectX_INCLUDE_DIR})
  link_directories(${DirectX_LIB_DIR})
  add_definitions("-DWIN32_LEAN_AND_MEAN -DNOMINMAX")
  aux_source_directory(ois/src/win32 ois_src)
ELSEIF(UNIX)
  aux_source_directory(ois/src/linux ois_src)
#  INCLUDE(FindSDL)
#  Find_Package ( SDL REQUIRED )

#  IF(SDL_FOUND)
#    #MESSAGE("SDL Found")
#    INCLUDE_DIRECTORIES(SDL_INCLUDE_DIR)
#  ELSE(SDL_FOUND)
#    MESSAGE("SDL not found, required for OIS!")
#  ENDIF(SDL_FOUND)
ELSEIF(APPLE)
ENDIF(WIN32)

add_library(ois SHARED ${ois_src} ${ois_headers})
windows_hacks(ois)

#SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES VERSION ${LIB_VERSION} SOVERSION ${LIB_MAJOR_VERSION})
#IF(WIN32)
#    SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES PREFIX "")
#ENDIF()

# platform - libs
IF(WIN32)
  target_link_libraries(ois "dxguid.lib dinput8.lib")
ELSEIF(UNIX)
  TARGET_LINK_LIBRARIES(ois ${SDL_LIBRARY})
ELSEIF(APPLE)
ENDIF(WIN32)
