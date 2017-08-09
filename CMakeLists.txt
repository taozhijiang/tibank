cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
#set(CMAKE_CXX_FLAGS "-Wall -Wconversion -Woverloaded-virtual -Wpointer-arith -Wshadow -Wwrite-strings -march=native " )

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 ")

set (NETASK_VERSION_MAJOR 0)
set (NETASK_VERSION_MINOR 85)
configure_file (
  "include/config.h.in"
  "../include/config.h" )

# mysqlcppconn如果使用C开发会在释放的时候段错误

add_definitions (-DBOOST_LOG_DYN_LINK)
aux_source_directory( source/ DIR_SRCS )

include_directories( include/ )
include_directories( source/ )
include_directories( source/commonutil )
include_directories( source/json)
include_directories( /usr/include/mysql )

add_subdirectory( source/commonutil )
add_subdirectory( source/json )


link_directories( /usr/lib64/mysql/ )

add_executable( tibank main.cpp ${DIR_SRCS} )

# ld iconv ?
set (EXTRA_LIBS ${EXTRA_LIBS} ssl event)
set (EXTRA_LIBS ${EXTRA_LIBS} pthread hiredis)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread-mt boost_date_time boost_regex)
set (EXTRA_LIBS ${EXTRA_LIBS} mysqlcppconn)
set (EXTRA_LIBS ${EXTRA_LIBS} commonutil json )

target_link_libraries( tibank ${EXTRA_LIBS} )

# /usr/local/bin/netask
install (TARGETS tibank DESTINATION bin )
# /usr/local/include/netask/
install (DIRECTORY include/ DESTINATION include/tibank)


# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_VERSION_MAJOR "${AIMLSRV_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${AIMLSRV_VERSION_MINOR}")
include (CPack)