# Configure Grass demo build

set(SOURCE_FILES src/Grass.cpp)

add_executable(Demo_Grass ${SOURCE_FILES})
target_link_libraries(Demo_Grass OgreMain ${OIS_LIBRARY})

