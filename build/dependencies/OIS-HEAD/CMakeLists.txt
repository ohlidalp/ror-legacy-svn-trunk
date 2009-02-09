aux_source_directory(ois/src/ ois_src)
FILE(GLOB_RECURSE ois_headers  ois/includes/*.h)
include_directories (ois/includes)

# general flags
add_definitions("-D_CRT_SECURE_NO_WARNINGS -DOIS_DYNAMIC_LIB -DOIS_NONCLIENT_BUILD")

SET(EnableFunctionLevelLinking TRUE)
SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "5")
SET(LIB_BUILD_VERSION "0")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_BUILD_VERSION}")
SET(LIB_INSTALL_DIR "lib")

IF(WIN32)
  add_definitions("-D_WIN32")
ELSEIF(UNIX)
ELSEIF(APPLE)
ENDIF(WIN32)

# platform
IF(WIN32)
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

install(TARGETS ois
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
      