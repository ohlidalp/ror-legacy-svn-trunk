project(RoR_Main)

FILE(GLOB_RECURSE ror_src  source/*.cpp source/*.c source/*.h)

include_directories (OgreMain/include Dependencies/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/mysocketw/)

if(CAELUM_BUILD_NEW)
  include_directories (${RoR_Dependencies_SOURCE_DIR}/caelum/main/include)
else(CAELUM_BUILD_NEW)
  include_directories (${RoR_Dependencies_SOURCE_DIR}/Caelum-2006-11-09/include)
endif(CAELUM_BUILD_NEW)

include_directories (${RoR_Dependencies_SOURCE_DIR}/openal-soft-1.6.372/include/AL)
include_directories (${RoR_Dependencies_SOURCE_DIR}/openal-soft-1.6.372/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/OIS-HEAD/ois/includes)
include_directories (${RoR_Dependencies_SOURCE_DIR}/moFileReader/include)
include_directories (${RoR_Dependencies_SOURCE_DIR}/rorserver-sf/source)
include_directories (${RoR_Dependencies_SOURCE_DIR}/mygui/include)

IF(WIN32)
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
ELSEIF(UNIX)
ELSEIF(APPLE)
ENDIF(WIN32)

# optional features
if(ROR_BUILD_HYDRAX)
  add_definitions("-DHYDRAX")
  include_directories (${RoR_Dependencies_SOURCE_DIR}/hydrax/)
endif(ROR_BUILD_HYDRAX)

if(ROR_BUILD_PAGED)
  add_definitions("-DPAGED")
  include_directories (${RoR_Dependencies_SOURCE_DIR}/PagedGeometry/include)
endif(ROR_BUILD_PAGED)

if(ROR_BUILD_BEAMSTATS)
  add_definitions("-DBEAMSTATS -DTIMING")
endif(ROR_BUILD_BEAMSTATS)

if(ROR_BUILD_PYTHONSCRIPT)
  add_definitions("-DPYTHONSCRIPT")
endif(ROR_BUILD_PYTHONSCRIPT)

if(ROR_BUILD_LUASCRIPT)
  add_definitions("-DLUASCRIPT")
  include_directories (${RoR_Dependencies_SOURCE_DIR}/lua-5.1.2/src)
endif(ROR_BUILD_LUASCRIPT)

if(ROR_BUILD_BUGTRAP)
  add_definitions("-DBUGTRAP")
  include_directories (${RoR_Dependencies_SOURCE_DIR}/bugtrap)
endif(ROR_BUILD_BUGTRAP)

if(CAELUM_BUILD_NEW)
  add_definitions("-DCAELUMNEW")
endif(CAELUM_BUILD_NEW)

if(ROR_BUILD_XFIRE)
  add_definitions("-DXFIRE")
endif(ROR_BUILD_XFIRE)

IF(ROR_BUILD_ANGELSCRIPT)
  add_definitions("-DANGELSCRIPT")
  include_directories (${RoR_Dependencies_SOURCE_DIR}/angelscript/include)
  include_directories (${RoR_Dependencies_SOURCE_DIR}/angelscript/add_on)
ENDIF(ROR_BUILD_ANGELSCRIPT)

if(ROR_BUILD_NEWNET)
  add_definitions("-DNEWNET")
  include_directories (${RoR_Dependencies_SOURCE_DIR}/raknet/Source)
  include_directories (${RoR_SOURCE_DIR}/rorserver_new)
  set(ror_src ${ror_src} ${RoR_SOURCE_DIR}/rorserver_new/nethelper.cpp ${RoR_SOURCE_DIR}/rorserver_new/nethelper.h)
endif(ROR_BUILD_NEWNET)

# general flags
add_definitions("-DOIS_HEAD")

# platform specific flags
IF(WIN32)
  include_directories (${RoR_Dependencies_SOURCE_DIR}/pthreads/Pre-built.2/include)

#  add_definitions("")
ELSEIF(UNIX)
#  some useful optimizations, including SSE
#  add_definitions("-mfpmath=sse -msse2 -O2 -fomit-frame-pointer -fstrict-aliasing -ffast-math -ftracer")
  add_definitions("-Wextra -Wfloat-equal -Wno-deprecated-declarations") # -Wunreachable-code")

  # ignore warnings that are generated from the libs we use, just warn about RoR code
  add_definitions("
    -isystem ${RoR_Dependencies_SOURCE_DIR}/ogrenew/OgreMain/include
  ")

  EXEC_PROGRAM(pkg-config ARGS "--cflags gtk+" OUTPUT_VARIABLE CFLAGS_GTK)
  SET(CMAKE_C_FLAGS "${CMAKE_PKGCONFIG_C_FLAGS} ${CFLAGS_GTK}")

ELSEIF(APPLE)
#  add_definitions("")
ENDIF(WIN32)

# main target
add_executable(RoR ${ror_src} source/icon.rc)

# add dependencies
add_dependencies(RoR OgreMain caelum mysocketw ois mygui mofilereader)

# optional features
if(ROR_BUILD_HYDRAX)
  add_dependencies(RoR hydrax)
endif(ROR_BUILD_HYDRAX)

if(ROR_BUILD_PAGED)
  add_dependencies(RoR paged)
endif(ROR_BUILD_PAGED)

if(ROR_BUILD_ETM)
  add_dependencies(RoR etm)
endif(ROR_BUILD_ETM)

if(ROR_BUILD_LUASCRIPT)
  add_dependencies(RoR lua)
endif(ROR_BUILD_LUASCRIPT)

if(ROR_BUILD_BUGTRAP)
  #
endif(ROR_BUILD_BUGTRAP)

# now the linking

# always required common libs for self generated targets
target_link_libraries(RoR
  caelum
  mysocketw
  OgreMain
  ois
  mygui
  mofilereader
)

IF(WIN32)
  set_target_properties(RoR PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
  set_target_properties(RoR PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
  set_target_properties(RoR PROPERTIES LINK_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:NOWIN98 /SUBSYSTEM:WINDOWS")
  if(ROR_USE_OPTIMIZATIONS_WINDOWS)
    SET_TARGET_PROPERTIES(RoR PROPERTIES COMPILE_FLAGS "/GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS-")
  else(ROR_USE_OPTIMIZATIONS_WINDOWS)
    SET_TARGET_PROPERTIES(RoR PROPERTIES COMPILE_FLAGS "/Od")
  endif(ROR_USE_OPTIMIZATIONS_WINDOWS)
  
  target_link_libraries(RoR openal32)
ELSEIF(UNIX)
  SET_TARGET_PROPERTIES(RoR PROPERTIES COMPILE_FLAGS "-msse2 -msse2 -mmmx -msse -msse3 -mssse3 -m3dnow")
  SET_TARGET_PROPERTIES(RoR PROPERTIES LINK_FLAGS    "")
  target_link_libraries(RoR openal)
ELSEIF(APPLE)
ENDIF(WIN32)


# optional, self build features
if(ROR_BUILD_HYDRAX)
  add_dependencies(RoR hydrax)
  target_link_libraries(RoR hydrax)
endif(ROR_BUILD_HYDRAX)

if(ROR_BUILD_PAGED)
  add_dependencies(RoR paged)
  target_link_libraries(RoR paged)
endif(ROR_BUILD_PAGED)

if(ROR_BUILD_LUASCRIPT)
  add_dependencies(RoR lua)
  target_link_libraries(RoR lua)
endif(ROR_BUILD_LUASCRIPT)

if(ROR_BUILD_NEWNET)
 add_dependencies(RoR RakNet)
 target_link_libraries(RoR RakNet)
endif(ROR_BUILD_NEWNET)

IF(ROR_BUILD_ANGELSCRIPT)
 add_dependencies(RoR angelscript)
 target_link_libraries(RoR angelscript)
ENDIF(ROR_BUILD_ANGELSCRIPT)


IF(WIN32)
ELSEIF(UNIX)
  EXEC_PROGRAM(pkg-config ARGS "--libs gtk+" OUTPUT_VARIABLE LDFLAGS_GTK)
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LDFLAGS_GTK}")
ELSEIF(APPLE)
ENDIF(WIN32)

# platform specific libraries (external ones only)
IF(WIN32)
  target_link_libraries(RoR
    ${RoR_Dependencies_SOURCE_DIR}/pthreads/Pre-built.2/lib/pthreadVC2.lib
  )
ELSEIF(UNIX)
  target_link_libraries(RoR
#    /usr/lib/libossp-uuid_dce.so
    /usr/lib/libzzip.so
    /usr/lib/libfreetype.so
    /usr/lib/libX11.so
    /usr/lib/libXt.so

    /usr/lib/libXaw.so
  )
ELSEIF(APPLE)
ENDIF(WIN32)

install(TARGETS RoR
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
