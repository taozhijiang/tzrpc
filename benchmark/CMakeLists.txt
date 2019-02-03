cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )

set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 ")


set (XTRA_RHEL6x /home/nicol/svnclient/Incubator/xtra_rhel6.x)
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

add_executable( perf_case_a perf_case_a.cpp)
add_executable( perf_case_b perf_case_b.cpp)

set (EXTRA_LIBS Client )

set (EXTRA_LIBS ${EXTRA_LIBS} ssl config++)
set (EXTRA_LIBS ${EXTRA_LIBS} pthread)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread boost_date_time boost_regex)
set (EXTRA_LIBS ${EXTRA_LIBS} protoc protobuf )


target_link_libraries( perf_case_a -lrt -rdynamic -ldl ${EXTRA_LIBS} )
target_link_libraries( perf_case_b -lrt -rdynamic -ldl ${EXTRA_LIBS} )

