cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )

set (INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/source/
        ${PROJECT_SOURCE_DIR}/deps/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/include/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/include/google/
)
include_directories( ${INCLUDE_DIRS} )

link_directories( /usr/lib64/mysql/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/libs/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/libs/boost/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/libs/google/
        ${PROJECT_SOURCE_DIR}/deps/xtra_rhelz.x/libs/google/protobuf-2.5.0/
        ../lib
)

aux_source_directory( ./ UNIT_TEST_SRCS )
add_executable( test ${UNIT_TEST_SRCS} )

set (EXTRA_LIBS Scaffold Protocol RPC Network Utils Client )

set (EXTRA_LIBS ${EXTRA_LIBS} ssl config++)
set (EXTRA_LIBS ${EXTRA_LIBS} pthread)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread boost_chrono boost_regex)
set (EXTRA_LIBS ${EXTRA_LIBS} protoc protobuf )


set (EXTRA_LIBS ${EXTRA_LIBS} gtest gmock)
target_link_libraries( test -lrt -rdynamic -ldl
    ${EXTRA_LIBS}
)

