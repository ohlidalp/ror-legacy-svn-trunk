# Configure DynTex demo build

set(SOURCE_FILES src/DynTex.cpp)

add_executable(Demo_DynTex ${SOURCE_FILES})
target_link_libraries(Demo_DynTex OgreMain ${OIS_LIBRARY})

