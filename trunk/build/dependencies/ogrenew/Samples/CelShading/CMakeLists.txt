# Configure CelShading demo build

set(SOURCE_FILES src/CelShading.cpp)

add_executable(Demo_CelShading ${SOURCE_FILES})
target_link_libraries(Demo_CelShading OgreMain ${OIS_LIBRARY})

