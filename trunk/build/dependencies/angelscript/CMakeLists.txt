FILE(GLOB_RECURSE angelscript_src  source/*.cpp)
FILE(GLOB_RECURSE angelscript_include  include/*.h)

FILE(GLOB_RECURSE angelscript_add_on_dict    add_on/scriptdictionary/*)
FILE(GLOB_RECURSE angelscript_add_on_math    add_on/scriptmath/*)
FILE(GLOB_RECURSE angelscript_add_on_string  add_on/scriptstring/*)

include_directories (include)
add_definitions("-D_CRT_SECURE_NO_WARNINGS -DANGELSCRIPT_EXPORT -D_LIB")

add_library(angelscript STATIC ${angelscript_src} ${angelscript_include} ${angelscript_add_on_dict} ${angelscript_add_on_math} ${angelscript_add_on_string})
