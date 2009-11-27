project(WSync)


cmake_minimum_required(VERSION 2.5)
include_directories (include)

find_package(Boost REQUIRED COMPONENTS filesystem iostreams system)
link_directories(${Boost_LIBRARY_DIRS})
include_directories (${Boost_INCLUDE_DIR})
if(Boost_FOUND)
else()
  message(" !!! boost not found!")
endif()

# first compile the lib, then the console tool
add_library(libwsync STATIC source/SHA1.cpp source/wsync.cpp include/wsync.h)
IF(WIN32)
  SET_TARGET_PROPERTIES(libwsync PROPERTIES COMPILE_FLAGS "/MP")
ENDIF(WIN32)

add_library(libwsync-static STATIC source/SHA1.cpp source/wsync.cpp include/wsync.h)
IF(WIN32)
  SET_TARGET_PROPERTIES(libwsync-static PROPERTIES COMPILE_FLAGS "/MP /MT")
ENDIF(WIN32)

IF(WIN32)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  add_executable(wsync source/main source/icon.rc)

  # UAC vista things to request admin access for the update
  SET_TARGET_PROPERTIES(wsync PROPERTIES LINK_FLAGS /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'")

ELSEIF(UNIX)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
  add_executable(wsync source/main)
ELSEIF(APPLE)
ENDIF(WIN32)

target_link_libraries(wsync libwsync)

IF(WIN32)
ELSEIF(UNIX)
  target_link_libraries (wsync
    ${Boost_LIBRARIES}
    pthread
  )

ELSEIF(APPLE)
ENDIF(WIN32)



if(BIN_DIR)
install(TARGETS wsync
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})

install(TARGETS libwsync
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})

endif(BIN_DIR)

