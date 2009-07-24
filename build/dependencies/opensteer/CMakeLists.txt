#FILE(GLOB_RECURSE sources  src/*.cpp include/*.h)

set(sources
	src/Camera.cpp
	src/Clock.cpp
	src/lq.c
	src/Pathway.cpp
	src/PlugIn.cpp
	src/SimpleVehicle.cpp
	src/Vec3.cpp
)
include_directories (include)

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-D_CONSOLE -D_DLL")

# the final lib
add_library(opensteer STATIC ${sources})

# this is for windows only, since windows is stupid ...

install(TARGETS opensteer
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
