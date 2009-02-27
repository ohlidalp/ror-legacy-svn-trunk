project(rorserver)
set(server_src rornetv2.h server.cpp nethelper.h nethelper.cpp)

include_directories (${RoR_Dependencies_SOURCE_DIR}/raknet/Source)

IF(WIN32)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
ENDIF(WIN32)

add_executable(rorserver ${server_src})

# add dependencies
add_dependencies(rorserver RakNet)

IF(WIN32)
  target_link_libraries(rorserver
    RakNet
    ${RoR_Dependencies_SOURCE_DIR}/pthreads/Pre-built.2/lib/pthreadVC2.lib
    ws2_32.lib
  )
ELSEIF(UNIX)
    target_link_libraries(rorserver RakNet pthread)
ELSEIF(APPLE)
ENDIF(WIN32)

install(TARGETS rorserver
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
