FILE(GLOB_RECURSE paged_src  source/*.cpp include/*.h)

include_directories (include)
include_directories (${OGRE_INCLUDE_DIR})

# general flags
add_definitions("-D_LIB")

# the final lib
add_library(paged STATIC ${paged_src})

# this is for windows only, since windows is stupid ...
add_dependencies(paged OgreMain)
target_link_libraries(paged OgreMain)

install(TARGETS paged
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})

