# Configure RenderToTexture demo build

set(SOURCE_FILES src/RenderToTexture.cpp)

add_executable(Demo_RenderToTexture ${SOURCE_FILES})
target_link_libraries(Demo_RenderToTexture OgreMain ${OIS_LIBRARY})

