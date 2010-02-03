#aux_source_directory(src lua_sources)
set(lua_sources
  src/lapi.c
  src/lcode.c
  src/ldebug.c
  src/ldo.c
  src/ldump.c
  src/lfunc.c
  src/lgc.c
  src/llex.c
  src/lmem.c
  src/lobject.c
  src/lopcodes.c
  src/lparser.c
  src/lstate.c
  src/lstring.c
  src/ltable.c
  src/ltm.c
  src/lundump.c
  src/lvm.c
  src/lzio.c
  src/lauxlib.c
  src/lbaselib.c
  src/ldblib.c
  src/liolib.c
  src/lmathlib.c
  src/loslib.c
  src/ltablib.c
  src/lstrlib.c
  src/loadlib.c
  src/linit.c
)

FILE(GLOB_RECURSE lua_headers  src/*.h)

# general flags
add_definitions("-D_LIB -D_CRT_SECURE_NO_WARNINGS")

# platform
IF(WIN32)
ELSEIF(UNIX)
	add_definitions("-DLUA_USE_LINUX")
ELSEIF(APPLE)
ENDIF(WIN32)

add_library(lua STATIC ${lua_sources} ${lua_headers})

