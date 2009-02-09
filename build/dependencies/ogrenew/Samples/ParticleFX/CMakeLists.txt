# Configure ParticleFX demo build

set(HEADER_FILES include/ParticleFX.h)
set(SOURCE_FILES src/ParticleFX.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_ParticleFX ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_ParticleFX OgreMain ${OIS_LIBRARY})

