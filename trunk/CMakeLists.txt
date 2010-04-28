# ensure cmake features we need
CMAKE_MINIMUM_REQUIRED( VERSION 2.6.0 )
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
# done with cmake check

project(RoR)

include(CMakeDependenciesConfig.txt)

#setup the default build mode

# global cmake options
SET(BUILD_SHARED_LIBS ON)

# setup paths
SET(ROR_DEPENDENCY_DIR       "${RoR_SOURCE_DIR}/source/dependencies")
SET(RUNTIME_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${RoR_SOURCE_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH   ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH      ${RUNTIME_OUTPUT_DIRECTORY})
SET(RoR_Dependencies_CMAKE_DIR "${ROR_DEPENDENCY_DIR}/cmake")

# set the tools binary directory accordingly to the used platform
IF(WIN32)
  SET(ROR_BINTOOL_DIR             ${RoR_SOURCE_DIR}/tools/windows/binaries)
ELSEIF(UNIX)
  SET(ROR_BINTOOL_DIR             ${RoR_SOURCE_DIR}/tools/linux/binaries)
ELSEIF(APPLE)
  SET(ROR_BINTOOL_DIR             ${RoR_SOURCE_DIR}/tools/linux/binaries)
ENDIF(WIN32)

# hide some settings
mark_as_advanced(
  CMAKE_DEBUG_POSTFIX
  CMAKE_BACKWARDS_COMPATIBILITY
  Boost_LIB_DIAGNOSTIC_DEFINITIONS
  EXECUTABLE_OUTPUT_PATH
  LIBRARY_OUTPUT_PATH
)

if(WIN32)
  # add multi processor compilation flags
  set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}        /MP /GL /Ox /Ob2 /Oi /Ot /Oy /fp:fast /GS- /MP /Zi")
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}     /MP /Zi")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /Od /Zi")
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}          /MP /Od /Zi")
  # some general flags
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  include_directories(${DirectX_INCLUDE_DIR})

  set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /DEBUG /SUBSYSTEM:CONSOLE")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /DEBUG /SUBSYSTEM:WINDOWS /LTCG /OPT:REF")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /DEBUG /SUBSYSTEM:CONSOLE /LTCG /OPT:REF")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG /SUBSYSTEM:CONSOLE /LTCG /OPT:REF")
ELSEIF(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE      "${CMAKE_EXE_LINKER_FLAGS_RELEASE}      -mfpmath=sse -msse2 -mmmx -msse -msse3 -m3dnow -O2 -fomit-frame-pointer -fstrict-aliasing -ffast-math -ftracer")
  set(CMAKE_EXE_LINKER_FLAGS_RelWithDebug "${CMAKE_EXE_LINKER_FLAGS_RelWithDebug} -O0")
endif(WIN32)

macro(windows_hacks NAME)
IF(WIN32)
	if (MSVC_IDE)
		# hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
		set_target_properties(${NAME} PROPERTIES PREFIX "../")
		set_target_properties(${NAME} PROPERTIES IMPORT_PREFIX "../")
	endif(MSVC_IDE)
ENDIF(WIN32)
endmacro(windows_hacks)

macro(copy_files srcDir destDir)
    message(STATUS "Configuring ${destDir} ...")
    #make_directory(${destDir})

    file(GLOB files RELATIVE ${srcDir} ${srcDir}/*)
    foreach(filename ${files})
        set(srcTemplatePath ${srcDir}/${filename})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Copying file ${filename}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${filename}
                    COPYONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(filename)
endmacro(copy_files)

add_subdirectory(source)

# fill the bin dir with binaries
copy_files(${ROR_BINTOOL_DIR} ${EXECUTABLE_OUTPUT_PATH})

if(WIN32)
	copy_files(${ROR_DEPENDENCIES_DIR}/bin ${EXECUTABLE_OUTPUT_PATH})
endif(WIN32)