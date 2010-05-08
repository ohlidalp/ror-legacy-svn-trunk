project(rorserializer)

add_definitions("-D_UNICODE -DNOLANG")

include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})

include_directories(.)
include_directories(../main)

# the lib
FILE(GLOB sources "*.cpp")
FILE(GLOB headers "*.h")
add_library(serializer STATIC ${sources} ${headers} ../main/BeamData.h)
windows_hacks(serializer)
target_link_libraries(serializer ${Ogre_LIBRARIES})

# the tests
FILE(GLOB test_sources "tests/*.cpp")
add_executable(serializer_test ${test_sources})
windows_hacks(serializer_test)
target_link_libraries(serializer_test ${Ogre_LIBRARIES} "serializer")




