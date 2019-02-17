cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )

set (XTRA_RHEL6x /home/nicol/workDir/svnclient/xtra_rhel6.x)
set (INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/source/
        ${XTRA_RHEL6x}/include/
        ${XTRA_RHEL6x}/include/google/
)

link_directories( /usr/lib64/mysql/
        ${XTRA_RHEL6x}/libs/
        ${XTRA_RHEL6x}/libs/boost/
        ${XTRA_RHEL6x}/libs/google/
        ${XTRA_RHEL6x}/libs/google/protobuf-2.5.0/
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

