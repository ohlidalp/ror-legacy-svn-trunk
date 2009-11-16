FILE(GLOB_RECURSE caelum_src src/*.cpp include/*.h)

include_directories (include ${OGRE_INCLUDE_DIR})

# the final lib
add_library(caelum SHARED ${caelum_src})

# this is for windows only, since windows is stupid ...
add_dependencies(caelum OgreMain)
target_link_libraries(caelum OgreMain)

install(TARGETS caelum
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
      
      