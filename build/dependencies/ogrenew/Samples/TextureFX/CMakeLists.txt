# Configure TextureFX demo build

set(HEADER_FILES include/TextureFX.h)
set(SOURCE_FILES src/TextureFX.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_TextureFX ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_TextureFX OgreMain ${OIS_LIBRARY})

