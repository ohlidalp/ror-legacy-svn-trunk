# Find ZZip includes and library
#
# This module defines
#  ZZip_INCLUDE_DIRS
#  ZZip_LIBRARIES, the libraries to link against to use ZZip.
#  ZZip_LIBRARY_DIRS, the location of the libraries
#  ZZip_FOUND, If false, do not try to use ZZip
#
# Copyright © 2007, Matt Williams
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (ZZip_LIBRARIES AND ZZip_INCLUDE_DIRS)
	SET(ZZip_FIND_QUIETLY TRUE) # Already in cache, be silent
ENDIF (ZZip_LIBRARIES AND ZZip_INCLUDE_DIRS)

IF (WIN32) #Windows
	SET(ZZip_INCLUDE_SEARCH_DIRS
		${ZZip_LIBRARY_SEARCH_DIRS}
		${CMAKE_INCLUDE_PATH}
		/usr/include/zzip
		/usr/local/include/zzip
		/opt/include/zzip
	)
	
	SET(ZZip_LIBRARY_SEARCH_DIRS
		${ZZip_LIBRARY_SEARCH_DIRS}
		${CMAKE_LIBRARY_PATH}
		/usr/lib
		/usr/local/lib
		/opt/lib
	)
	FIND_PATH(ZZip_INCLUDE_DIRS zzip.h ${ZZip_INCLUDE_SEARCH_DIRS})
	FIND_LIBRARY(ZZip_LIBRARIES zzip PATHS ${ZZip_LIBRARY_SEARCH_DIRS})
ELSE (WIN32) #Unix
	CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7 FATAL_ERROR)
	FIND_PACKAGE(PkgConfig)
	PKG_SEARCH_MODULE(ZZip zziplib)
	SET(ZZip_INCLUDE_DIRS ${ZZip_INCLUDE_DIRS})
	SET(ZZip_LIBRARY_DIRS ${ZZip_LIBDIR})
	SET(ZZip_LIBRARIES ${ZZip_LIBRARIES} CACHE STRING "")
ENDIF (WIN32)

#Do some preparation
SEPARATE_ARGUMENTS(ZZip_INCLUDE_DIRS)
SEPARATE_ARGUMENTS(ZZip_LIBRARIES)

SET(ZZip_INCLUDE_DIRS ${ZZip_INCLUDE_DIRS})
SET(ZZip_LIBRARIES ${ZZip_LIBRARIES})
SET(ZZip_LIBRARY_DIRS ${ZZip_LIBRARY_DIRS})

MARK_AS_ADVANCED(ZZip_INCLUDE_DIRS ZZip_LIBRARIES ZZip_LIBRARY_DIRS)

IF (ZZip_INCLUDE_DIRS AND ZZip_LIBRARIES)
	SET(ZZip_FOUND TRUE)
ENDIF (ZZip_INCLUDE_DIRS AND ZZip_LIBRARIES)

IF (ZZip_FOUND)
	IF (NOT ZZip_FIND_QUIETLY)
		MESSAGE(STATUS "  libraries : ${ZZip_LIBRARIES} from ${ZZip_LIBRARY_DIRS}")
		MESSAGE(STATUS "  includes  : ${ZZip_INCLUDE_DIRS}")
	ENDIF (NOT ZZip_FIND_QUIETLY)
ELSE (ZZip_FOUND)
	IF (ZZip_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find ZZip")
	ENDIF (ZZip_FIND_REQUIRED)
ENDIF (ZZip_FOUND)
