# Configure Dot3Bump demo build

set(SOURCE_FILES src/Dot3Bump.cpp)

add_executable(Demo_Dot3Bump ${SOURCE_FILES})
target_link_libraries(Demo_Dot3Bump OgreMain ${OIS_LIBRARY})

