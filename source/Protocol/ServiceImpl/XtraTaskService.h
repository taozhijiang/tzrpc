#ifndef __PROTOCOL_XTRA_TASK_SERVICE_H__
#define __PROTOCOL_XTRA_TASK_SERVICE_H__

#include <xtra_rhel6.h>

#include <RPC/Service.h>
#include <RPC/RpcInstance.h>

#include <Scaffold/ConfHelper.h>

#include <Protocol/gen-cpp/XtraTask.pb.h>

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

    bool init();

private:
    struct DetailExecutorConf {

        // 用来返回给Executor使用的，主要是线程伸缩相关的东西
        ExecutorConf executor_conf_;

        // other stuffs if needed, please add here

    };

    std::mutex conf_lock_;
    std::shared_ptr<DetailExecutorConf> conf_ptr_;

    bool handle_rpc_service_conf(const libconfig::Setting& setting);
    bool handle_rpc_service_runtime_conf(const libconfig::Setting& setting);

    ExecutorConf get_executor_conf();
    int update_runtime_conf(const libconfig::Config& conf);
    int module_status(std::string& strModule, std::string& strKey, std::string& strValue);


private:

    ////////// RPC handlers //////////
    void read_ops_impl(std::shared_ptr<RpcInstance> rpc_instance);
    void write_ops_impl(std::shared_ptr<RpcInstance> rpc_instance);

    const std::string instance_name_;

};

} // namespace tzrpc

#endif // __PROTOCOL_XTRA_TASK_SERVICE_H__
