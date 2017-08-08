cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")


include_directories( ../../include/ )

aux_source_directory(. DIR_LIB_SRCS)
add_library (json STATIC ${DIR_LIB_SRCS})

install (TARGETS json
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib/static )
