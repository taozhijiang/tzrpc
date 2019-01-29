cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x " )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wpointer-arith -Wwrite-strings -Werror=format -march=native " )


####################
### 修改编译选项 ###

# if what to see debug in verbose mode
# cmake -DCMAKE_VERBOSE_MAKEFILE=true ../

#set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 ")

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
ELSEIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    add_definitions(-DNP_DEBUG)
ELSE(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(FATAL_ERROR "Unknown BUILD_TYPE ... ")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)


###################


# mysqlcppconn如果使用C开发会在释放的时候段错误
set (XTRA_RHEL6x /home/nicol/workDir/Incubator/xtra_rhel6.x)
set (PROTOBUF_GEN_CMD "cp -fr ${XTRA_RHEL6x}/bin/protoc-2.5.0 ${PROJECT_SOURCE_DIR}/bin/protoc && chmod a+x ${PROJECT_SOURCE_DIR}/bin/protoc && cd ${CMAKE_CURRENT_SOURCE_DIR}/source/Protocol/source && rm -fr ../gen-cpp && mkdir -p ../gen-cpp && make ")
exec_program(${PROTOBUF_GEN_CMD})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


set (INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/source/
        ${XTRA_RHEL6x}/include/
        ${XTRA_RHEL6x}/include/google/
)
include_directories( ${INCLUDE_DIRS} )
	
add_subdirectory( source/RPC )
add_subdirectory( source/Utils )
add_subdirectory( source/Scaffold )
add_subdirectory( source/Protocol )
add_subdirectory( source/Network )

link_directories(
        /usr/lib64/mysql/
        ${XTRA_RHEL6x}/libs/
        ${XTRA_RHEL6x}/libs/boost/
        ${XTRA_RHEL6x}/libs/google/
        ${XTRA_RHEL6x}/libs/google/protobuf-2.5.0/
)

# local temp generated file
exec_program( "export BUILD_VAR=`git log -1 --pretty=%H` && echo 'const char *build_commit_version = \"VCS: Commit:' $BUILD_VAR '\";' > build_version.cpp ")
exec_program( "export BUILD_VAR=`git symbolic-ref HEAD` && echo 'const char *build_commit_branch = \"VCS: Branch:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`git log -1 --pretty=%cd` && echo 'const char *build_commit_date = \"VCS: Date:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`git log -1 --pretty=\"%an %ae\"` && echo 'const char *build_commit_author = \"VCS: Author:' $BUILD_VAR '\";' >> build_version.cpp ")
exec_program( "export BUILD_VAR=`date` && echo 'const char *build_time = \"Build At:' $BUILD_VAR '\";' >> build_version.cpp ")

add_executable( pbi_bankpay_lookup_service main.cpp build_version.cpp ${DIR_SRCS} )

# ld iconv ?

set (EXTRA_LIBS Scaffold Protocol RPC Network Utils )

set (EXTRA_LIBS ${EXTRA_LIBS} ssl config++)
set (EXTRA_LIBS ${EXTRA_LIBS} pthread)
set (EXTRA_LIBS ${EXTRA_LIBS} boost_system boost_thread boost_date_time boost_regex)
set (EXTRA_LIBS ${EXTRA_LIBS} protoc protobuf )



# 某些库需要静态全连接，否则主服务没使用但是so使用，会报符号没有找到
target_link_libraries( pbi_bankpay_lookup_service -lrt -rdynamic -ldl
    -Wl,--whole-archive ${WHOLE_LIBS} -Wl,--no-whole-archive
    ${EXTRA_LIBS}
)

add_subdirectory( test )
