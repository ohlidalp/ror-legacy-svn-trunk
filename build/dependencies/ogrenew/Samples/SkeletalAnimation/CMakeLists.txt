# Configure SkeletalAnimation demo build

set(HEADER_FILES include/SkeletalAnimation.h)
set(SOURCE_FILES src/SkeletalAnimation.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_SkeletalAnimation ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_SkeletalAnimation OgreMain ${OIS_LIBRARY})

