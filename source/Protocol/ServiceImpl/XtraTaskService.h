#include <xtra_rhel6.h>

#include <RPC/Service.h>
#include <RPC/RpcInstance.h>

#include <Protocol/gen-cpp/XtraTask.pb.h>

#ifndef __PROTOCOL_XTRA_TASK_SERVICE_H__
#define __PROTOCOL_XTRA_TASK_SERVICE_H__

namespace tzrpc {

class XtraTaskService : public Service {

public:
    explicit XtraTaskService(const std::string& instance_name):
        instance_name_(instance_name) {
    }

    ~XtraTaskService() {}

    void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance);
    std::string instance_name() {
        return instance_name_;
    }

private:

    ////////// RPC handlers //////////
    void read_ops_impl(std::shared_ptr<RpcInstance> rpc_instance);
    void write_ops_impl(std::shared_ptr<RpcInstance> rpc_instance);

    const std::string instance_name_;

};

} // namespace tzrpc

#endif // __PROTOCOL_XTRA_TASK_SERVICE_H__
