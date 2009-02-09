# Configure CubeMapping demo build

set(HEADER_FILES include/CubeMapping.h)
set(SOURCE_FILES src/CubeMapping.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_CubeMapping ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_CubeMapping OgreMain ${OIS_LIBRARY})

