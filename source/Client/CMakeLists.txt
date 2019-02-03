cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")


include_directories( ${PROJECT_SOURCE_DIR}/include/
                     ${PROJECT_SOURCE_DIR}/source/Client/
                     ${PROJECT_SOURCE_DIR}/source/Client/include )

aux_source_directory(. DIR_LIB_SRCS)
add_library (Client STATIC ${DIR_LIB_SRCS})

install (TARGETS Client
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib/static )
