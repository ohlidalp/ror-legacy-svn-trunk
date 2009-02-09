# Configure Fresnel demo build

set(SOURCE_FILES src/Fresnel.cpp)

add_executable(Demo_Fresnel ${SOURCE_FILES})
target_link_libraries(Demo_Fresnel OgreMain ${OIS_LIBRARY})

