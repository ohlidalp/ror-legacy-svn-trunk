# Configure CameraTrack demo build

set(SOURCE_FILES src/CameraTrack.cpp)

add_executable(Demo_CameraTrack ${SOURCE_FILES})
target_link_libraries(Demo_CameraTrack OgreMain ${OIS_LIBRARY})

