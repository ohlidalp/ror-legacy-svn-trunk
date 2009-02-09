# Configure Transparency demo build

set(HEADER_FILES include/Transparency.h)
set(SOURCE_FILES src/Transparency.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_Transparency ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_Transparency OgreMain ${OIS_LIBRARY})

