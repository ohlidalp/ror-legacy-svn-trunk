# Configure Terrain demo build

set(HEADER_FILES include/Terrain.h)
set(SOURCE_FILES src/Terrain.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_Terrain ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_Terrain OgreMain ${OIS_LIBRARY})

