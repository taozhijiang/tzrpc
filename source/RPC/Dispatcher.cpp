/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <Utils/Log.h>

#include <Scaffold/ConfHelper.h>

#include <RPC/Service.h>
#include <RPC/Executor.h>
#include <RPC/RpcInstance.h>

#include <RPC/Dispatcher.h>

namespace tzrpc {

Dispatcher& Dispatcher::instance() {
    static Dispatcher dispatcher;
    return dispatcher;
}

bool Dispatcher::init() {

    initialized_ = true;

    for (auto iter = services_.begin(); iter != services_.end(); ++iter) {
        Executor* executor = dynamic_cast<Executor *>(iter->second.get());

        SAFE_ASSERT(executor);
        if (!executor) {
            log_err("dynamic cast failed for %s", iter->second->instance_name().c_str());
            return false;
        }

        executor->executor_start();
        log_debug("start executor: %s",  executor->instance_name().c_str());
    }

    // 注册配置动态配置更新接口，由此处分发到各个虚拟主机，不再每个虚拟主机自己注册
    ConfHelper::instance().register_runtime_callback(
        "Dispatcher",
        std::bind(&Dispatcher::module_runtime, this,
                  std::placeholders::_1));

    return true;
}

void Dispatcher::register_service(uint16_t service_id, std::shared_ptr<Service> service) {

    if (initialized_) {
        log_err("Dispatcher has already been initialized, does not support dynamic registerService");
        return;
    }

    auto exec_service = std::make_shared<Executor>(service);
    if (!exec_service || !exec_service->init()) {
        log_err("service %s init failed.", service->instance_name().c_str());
        return;
    }

    services_[service_id] = exec_service;
    log_debug("successful register service %s ", service->instance_name().c_str());
}


void Dispatcher::handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) {

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



int Dispatcher::module_runtime(const libconfig::Config& conf) {

    int ret_sum = 0;
    int ret = 0;

    for (auto iter = services_.begin(); iter != services_.end(); ++iter) {

        auto executor = iter->second;
        ret = executor->module_runtime(conf);
        log_notice("update_runtime_conf for host %s return: %d",
                   executor->instance_name().c_str(), ret);
        ret_sum += ret;
    }

    return ret_sum;
}

} // end namespace tzrpc

