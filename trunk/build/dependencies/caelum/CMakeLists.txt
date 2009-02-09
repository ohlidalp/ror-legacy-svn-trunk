FILE(GLOB_RECURSE caelum_new_src main/src/*.cpp main/include/*.h)

include_directories (main/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include)

# general flags
add_definitions("-DCAELUM_LIB")

# the final lib
add_library(caelum_new SHARED ${caelum_new_src})

install(TARGETS caelum_new
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})

# some caelum samples
add_subdirectory("samples")