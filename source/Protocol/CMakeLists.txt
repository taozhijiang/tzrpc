cmake_minimum_required (VERSION 2.8.11)

SET(SUBDIRS gen-cpp ServiceImpl)
foreach(dir ${SUBDIRS})
    aux_source_directory(${dir} CUR_RAFT_DIR_SRCS)
    SET(PROTOCOL_SRCS ${RAFT_SRCS} ${CUR_RAFT_DIR_SRCS})
endforeach()

# ld iconv ?

message("Full Protocol Sources: ${PROTOCOL_SRCS}")
add_library (Protocol STATIC ${PROTOCOL_SRCS})
