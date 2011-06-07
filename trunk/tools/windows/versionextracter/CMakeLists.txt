cmake_minimum_required (VERSION 2.6)
project (versionextracter)
add_executable (versionextracter main.cpp)
target_link_libraries (versionextracter Version.lib)