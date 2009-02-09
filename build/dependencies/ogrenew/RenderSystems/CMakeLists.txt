# Configure RenderSystem build

if (OGRE_BUILD_RENDERSYSTEM_D3D9)
  add_subdirectory(Direct3D9)
endif (OGRE_BUILD_RENDERSYSTEM_D3D9)

if (OGRE_BUILD_RENDERSYSTEM_GL)
  add_subdirectory(GL)
endif (OGRE_BUILD_RENDERSYSTEM_GL)

