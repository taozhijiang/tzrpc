### tzrpc

***tzrpc*** is a C++ RPC framework, in order to better supporting network client and service development. This project is mainly inspired by [logcabin](https://github.com/logcabin/logcabin), and has already been used in some projects.   
In fact, Apache Thrift is a nice RPC framework, it is stable, powerful and easy to use, but our own RPC framework, we can do lots of own controls, such as drop oldest or new request when queue is overloaded, create new or reduce task work threads according to overload, and many more features can be added in your own preferences.   

### Based on:
1. boost::asio async and sync network library, so the network concurrent and performance is nice.   
2. protobuf message for marshalling/unmarshalling. The network message and application message are separated, so other package protocols can also work, just for protobuf is so widespread.   
3. gtest unit_test framework.   