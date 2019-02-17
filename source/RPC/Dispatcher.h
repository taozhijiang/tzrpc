/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

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

        // 创建io_service工作线程
        io_service_thread_ = boost::thread(std::bind(&Dispatcher::io_service_run, this));

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
        ConfHelper::instance().register_conf_callback(
            std::bind(&Dispatcher::update_runtime_conf, this,
                      std::placeholders::_1));

        return true;
    }

    void register_service(uint16_t service_id, std::shared_ptr<Service> service) {

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

    io_service& get_io_service() {
        return  io_service_;
    }

    int update_runtime_conf(const libconfig::Config& conf);

private:

    Dispatcher():
        initialized_(false),
        services_({}),
        io_service_thread_(),
        io_service_(),
        work_guard_(new io_service::work(io_service_)){
    }

    ~Dispatcher() {
        work_guard_.reset();
    }
    bool initialized_;

    // 系统在启动的时候进行注册初始化，然后再提供服务，所以
    // 这边就不使用锁结构进行保护了，防止影响性能
    std::map<uint16_t, std::shared_ptr<Service>> services_;


    // 再启一个io_service_，主要使用DIspatcher单例和boost::asio异步框架
    // 来处理定时器等常用服务
    boost::thread io_service_thread_;
    io_service io_service_;

    // io_service如果没有任务，会直接退出执行，所以需要
    // 一个强制的work来持有之
    std::unique_ptr<io_service::work> work_guard_;
    void io_service_run() {

        log_notice("Dispatcher io_service thread running...");

        // io_service would not have had any work to do,
        // and consequently io_service::run() would have returned immediately.

        boost::system::error_code ec;
        io_service_.run(ec);

        log_notice("Dispatcher io_service thread terminated ...");
    }

};

} // end namespace tzrpc


#endif // __RPC_SERVICE_DISPATCHER_H__
