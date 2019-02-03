***tzrpc*** is a C++ RPC framework, in order to better support network client and service development. This project is mainly inspired by [logcabin](https://github.com/logcabin/logcabin).   

Based on:    
1. boost::asio async and sync network library.   
2. protobuf message for marshalling/unmarshalling.   
3. gtest unit_test framework.   


TODO:
1. 最大并发控制和过载保护;
2. 线程池根据负载自动伸缩(主线程？);
