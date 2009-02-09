# Configure Smoke demo build

set(HEADER_FILES include/Smoke.h)
set(SOURCE_FILES src/Smoke.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_Smoke ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_Smoke OgreMain ${OIS_LIBRARY})

