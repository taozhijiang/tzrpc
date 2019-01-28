#ifndef __RPC_SERVICE_H__
#define __RPC_SERVICE_H__

#include <boost/noncopyable.hpp>

#include <string>

// real rpc should implement this interface class

namespace tzrpc {

class RpcInstance;

class Service: public boost::noncopyable {

public:
    Service() {}
    ~Service() {}

    // 根据opCode分发rpc请求的处理
    virtual void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) = 0;
    virtual std::string instance_name() = 0;
};

} // end tzrpc

#endif // __RPC_SERVICE_H__
