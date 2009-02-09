# Configure Water demo build

set(HEADER_FILES include/WaterMesh.h)
set(SOURCE_FILES src/Water.cpp src/WaterMesh.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_Water ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_Water OgreMain ${OIS_LIBRARY})

