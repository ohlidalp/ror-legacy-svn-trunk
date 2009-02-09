# Configure Instancing demo build

set(HEADER_FILES include/Instancing.h)
set(SOURCE_FILES src/Instancing.cpp) 

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common/CEGUIRenderer/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(Demo_Instancing ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Demo_Instancing OgreMain ${CEGUI_LIBRARY} CEGUIOgreRenderer ${OIS_LIBRARY})

