# Configure Direct3D10 RenderSystem build

FILE(GLOB_RECURSE SOURCE_FILES *.cpp *.c *.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${DirectX_INCLUDE_DIR})
link_directories(${DirectX_LIBRARY_PATHS})
add_definitions(-D_USRDLL)

add_library(RenderSystem_Direct3D10 ${OGRE_LIB_TYPE} ${SOURCE_FILES})

target_link_libraries(RenderSystem_Direct3D10
  OgreMain
  d3d10
  d3dx10
  dxerr
  dxguid
  dxgi
)

if (NOT OGRE_STATIC)
  set_target_properties(RenderSystem_Direct3D10 PROPERTIES
    COMPILE_DEFINITIONS OGRED3DENGINEDLL_EXPORTS
  )
endif ()
set_target_properties(RenderSystem_Direct3D10 PROPERTIES PREFIX "")
windows_hacks(RenderSystem_Direct3D10)
