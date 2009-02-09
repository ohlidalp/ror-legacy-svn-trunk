# Configure SkyBox demo build

set(HEADER_FILES include/SkyBox.h)
set(SOURCE_FILES src/SkyBox.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_SkyBox ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_SkyBox OgreMain ${OIS_LIBRARY})

