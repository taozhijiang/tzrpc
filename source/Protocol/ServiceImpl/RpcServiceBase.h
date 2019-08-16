#ifndef __PROTOCOL_RPC_SERVICE_BASE_H__
#define __PROTOCOL_RPC_SERVICE_BASE_H__

#include <string>

#include <scaffold/Setting.h>
#include <other/Log.h>

namespace tzrpc {

// 继承一些实现函数，避免多个服务实例中重复执行
class RpcServiceBase {

protected:
    explicit RpcServiceBase(const std::string& instance_name) :
        instance_name_(instance_name) {
    }

    ~RpcServiceBase() { }

    std::string instance_name() {
        return instance_name_;
    }

    // Executor部分
    int handle_rpc_service_conf(const libconfig::Setting& setting, ExecutorConf& conf) {

        setting.lookupValue("exec_thread_pool_size",      conf.exec_thread_number_);
        setting.lookupValue("exec_thread_pool_size_hard", conf.exec_thread_number_hard_);
        setting.lookupValue("exec_thread_pool_step_size", conf.exec_thread_step_size_);

        // 检查ExecutorConf参数合法性
        if (conf.exec_thread_number_hard_ < conf.exec_thread_number_) {
            conf.exec_thread_number_hard_ = conf.exec_thread_number_;
        }

        if (conf.exec_thread_number_ <= 0 ||
            conf.exec_thread_number_ > 100 ||
            conf.exec_thread_number_hard_ > 100 ||
            conf.exec_thread_number_hard_ < conf.exec_thread_number_) {
            roo::log_err("Detected invalid exec_thread setting: normal %d, hard %d.",
                         conf.exec_thread_number_, conf.exec_thread_number_hard_);
            return -1;
        }

        // 可以为0，表示不进行动态更新
        if (conf.exec_thread_step_size_ < 0) {
            roo::log_err("Detected invalid exec_thread_step_size setting: %d.", conf.exec_thread_step_size_);
            return -1;
        }

        return 0;

    }

    int module_status(std::string& module, std::string& name, std::string& val) {

        // empty status ...
        return 0;
    }

private:

    const std::string instance_name_;

};

} // namespace tzrpc

#endif // __PROTOCOL_RPC_SERVICE_BASE_H__
