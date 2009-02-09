# Configure Gui demo build

set(SOURCE_FILES src/Gui.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common/CEGUIRenderer/include)
add_executable(Demo_Gui ${SOURCE_FILES})
target_link_libraries(Demo_Gui OgreMain ${CEGUI_LIBRARY} CEGUIOgreRenderer ${OIS_LIBRARY})

