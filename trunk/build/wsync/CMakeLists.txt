project(WSync)

cmake_minimum_required(VERSION 2.5)
include_directories (include)

IF(WIN32)
  set(WSYNC_BOOST_DIR "C:\\Program Files\\boost\\boost_1_36_0" CACHE STRING "set the path to your boost directory")
  include_directories(${WSYNC_BOOST_DIR})
  add_definitions("-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINDOWS -D_WIN32")
  link_directories(${WSYNC_BOOST_DIR}/lib)
ELSEIF(UNIX)
  add_definitions("-DNWIN32")
  link_directories(/usr/lib64)
  include_directories (/usr/include)
ELSEIF(APPLE)
ENDIF(WIN32)

# first compile the lib, then the console tool
add_library(libwsync STATIC source/SHA1.cpp source/wsync.cpp include/wsync.h)
add_executable(wsync source/main source/icon.rc)

target_link_libraries(wsync libwsync)

IF(WIN32)
ELSEIF(UNIX)
  target_link_libraries (wsync 
    libboost_filesystem.so
    libboost_iostreams.so
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

