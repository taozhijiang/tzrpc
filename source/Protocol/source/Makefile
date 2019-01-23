IDLS=$(wildcard *.proto)
OBJS=$(IDLS:%.proto=../gen-cpp/%.pb.cc)

all: ${OBJS}
	@echo "remove ../gen-cpp/*.skeleton.cpp files"

../gen-cpp/%.pb.cc: %.proto
	../../../bin/protoc -I. --cpp_out=../gen-cpp $^
