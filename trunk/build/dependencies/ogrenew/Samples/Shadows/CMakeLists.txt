# Configure Shadows demo build

set(SOURCE_FILES src/Shadows.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common/CEGUIRenderer/include)
add_executable(Demo_Shadows ${SOURCE_FILES})
target_link_libraries(Demo_Shadows OgreMain ${CEGUI_LIBRARY} CEGUIOgreRenderer ${OIS_LIBRARY})

