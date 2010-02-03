FILE(GLOB_RECURSE socketw_src *.cxx)
FILE(GLOB_RECURSE socketw_headers *.h)

# general flags
add_definitions("-DMYSOCKETW_EXPORTS -D_USRDLL")

# the final lib
add_library(mysocketw SHARED ${socketw_src} ${socketw_headers})

# libraries
IF(WIN32)
	target_link_libraries(mysocketw Ws2_32.lib)
ELSEIF(UNIX)
ELSEIF(APPLE)
ENDIF(WIN32)
