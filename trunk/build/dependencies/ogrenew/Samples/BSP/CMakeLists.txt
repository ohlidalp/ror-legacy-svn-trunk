# Configure BSP demo build

set(SOURCE_FILES src/BSP.cpp)

add_executable(Demo_BSP ${SOURCE_FILES})
target_link_libraries(Demo_BSP OgreMain ${OIS_LIBRARY})

