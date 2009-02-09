FILE(GLOB_RECURSE mofilereader_src  src/*.cpp include/*.h)

include_directories (include)

# -DHYDRAX_LIB will  give us the correct dllspec export setting
add_definitions("-D_CONSOLE")

# the final lib
add_library(mofilereader STATIC ${mofilereader_src})

# this is for windows only, since windows is stupid ...

install(TARGETS mofilereader
      RUNTIME DESTINATION ${BIN_DIR}
      LIBRARY DESTINATION ${BIN_DIR}
      ARCHIVE DESTINATION ${LIB_DIR})
