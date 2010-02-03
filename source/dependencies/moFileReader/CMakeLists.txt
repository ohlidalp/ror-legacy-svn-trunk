FILE(GLOB_RECURSE mofilereader_src  src/*.cpp include/*.h)

include_directories (include)

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-D_CONSOLE")

# the final lib
add_library(mofilereader STATIC ${mofilereader_src})
