FILE(GLOB_RECURSE hydrax_src  Hydrax/*.cpp Hydrax/*.h)

include_directories (Hydrax)

include_directories (${OGRE_INCLUDE_DIR})

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-DHYDRAX_LIB")

# the final lib
add_library(hydrax SHARED ${hydrax_src})

IF(WIN32)
  SET_TARGET_PROPERTIES(hydrax PROPERTIES COMPILE_FLAGS "/MP")
ENDIF(WIN32)

# this is for windows only, since windows is stupid ...
add_dependencies(hydrax OgreMain)
target_link_libraries(hydrax OgreMain)

install(TARGETS hydrax
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
      