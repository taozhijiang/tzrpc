#ifndef __RPC_DISPATCHER_H__
#define __RPC_DISPATCHER_H__

#include <cinttypes>
#include <memory>

#include <map>
#include <mutex>


#include <boost/noncopyable.hpp>

#include <RPC/Service.h>
#include <RPC/Executor.h>
#include <RPC/RpcInstance.h>

#include <Utils/Log.h>


namespace tzrpc {

class Dispatcher: public boost::noncopyable {

public:
    static Dispatcher& instance();

    bool init() {

        initialized_ = true;

        for (auto iter = services_.begin(); iter != services_.end(); ++iter) {
            Executor* executor = dynamic_cast<Executor *>(iter->second.get());
            SAFE_ASSERT(executor);

            executor->executor_start();
            log_debug("start executor: %s",  executor->instance_name().c_str());
        }

        return true;
    }

    void register_service(uint16_t service_id, std::shared_ptr<Service> service,
                          uint32_t thread_num ) {
        return register_service(service_id, service, thread_num, thread_num);
    }

    void register_service(uint16_t service_id, std::shared_ptr<Service> service,
                          uint32_t thread_num,
                          uint32_t thread_num_boost ) {

        if (initialized_) {
            log_err("Dispatcher has already been initialized, does not support dynamic registerService");
            return;
        }

        if (thread_num == 0 || thread_num > 100 ||
            thread_num > thread_num_boost ||
            thread_num_boost > 100 ) {
            log_err("invalid thread_num provided: %u, %s", thread_num, thread_num_boost);
            return;
        }
        auto exec_service = std::make_shared<Executor>(service, thread_num, thread_num_boost);
        if (!exec_service->init()) {
            log_err("service %s init failed.", service->instance_name().c_str());
            return;
        }

        services_[service_id] = exec_service;
        log_debug("successful register service %s ", service->instance_name().c_str());
    }

    void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) {

        if (!rpc_instance->validate_request()) {
            log_err("validate RpcInstance failed.");
            rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
            return;
        }

        std::shared_ptr<Service> service;
        auto it = services_.find(rpc_instance->get_service_id());
        if (it != services_.end())
            service = it->second;

        if (service) {
            service->handle_RPC(rpc_instance);
        } else {
            log_err("found service_impl for %u:%u failed.",
                    rpc_instance->get_service_id(),  rpc_instance->get_opcode());
            rpc_instance->reject(RpcResponseStatus::INVALID_SERVICE);
        }
    }

    std::string instance_name() {
        return "Dispatcher";
    }

private:

    Dispatcher():
        initialized_(false),
        services_({}) {
    }

    bool initialized_;

    // 系统在启动的时候进行注册初始化，然后再提供服务，所以
    // 这边就不使用锁结构进行保护了，防止影响性能
    std::map<uint16_t, std::shared_ptr<Service>> services_;

};

} // tzrpc


#endif // __RPC_SERVICE_DISPATCHER_H__
