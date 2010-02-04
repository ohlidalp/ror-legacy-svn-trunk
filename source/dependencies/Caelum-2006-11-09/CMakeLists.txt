FILE(GLOB_RECURSE caelum_src src/*.cpp include/*.h)

include_directories (include ${OGRE_INCLUDE_DIR})

# the final lib
add_library(caelum SHARED ${caelum_src})
windows_hacks(caelum)

# this is for windows only, since windows is stupid ...
add_dependencies(caelum OgreMain)
target_link_libraries(caelum OgreMain)
