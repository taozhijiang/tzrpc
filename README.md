### tzrpc

***tzrpc*** is a C++ RPC framework, in order to better supporting network client and service development. This project is mainly inspired by [logcabin](https://github.com/logcabin/logcabin), and has already been used in some projects.   
In fact, Apache Thrift is a nice RPC framework, it is stable, powerful and also easy to use. But build our own RPC framework, we can do may more controls and modifications, such as drop oldest or new request when queue is overloaded, create new or reduce task work threads according to overload atomaticlly, async client support, and many more features can be added in your own preferences.   

### Based on:
1. boost::asio async and sync network library, so the network concurrent and performance is nice.   
2. protobuf message for marshalling/unmarshalling. The network message and application message are represented seperately, so other package protocols can also work, this choice is just for protobuf is so widespread.   
3. gtest unit_test framework.   

### Note:   
The v1.3.x branch is legacy implementation, no features and improvements will be added any more.   
The master branch depends on Roo library, and will be actively maintained.   
