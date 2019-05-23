/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __RPC_EXECUTOR_H__
#define __RPC_EXECUTOR_H__

#include <xtra_rhel.h>

#include <other/Log.h>

#include <container/EQueue.h>
#include <concurrency/ThreadPool.h>
#include <scaffold/Setting.h>

#include <RPC/Service.h>


namespace tzrpc {


class Executor : public Service,
    public std::enable_shared_from_this<Executor> {

public:

    explicit Executor(std::shared_ptr<Service> service_impl) :
        service_impl_(service_impl),
        rpc_queue_(),
        conf_lock_(),
        conf_({ }) {
    }

    void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance)override {
        rpc_queue_.PUSH(rpc_instance);
    }

    std::string instance_name() {
        return service_impl_->instance_name();
    }

    bool init();
    int module_runtime(const libconfig::Config& conf);
    int module_status(std::string& module, std::string& name, std::string& val);

private:
    std::shared_ptr<Service> service_impl_;
    roo::EQueue<std::shared_ptr<RpcInstance>> rpc_queue_;

private:
    // 这个锁保护conf_使用的，因为使用频率不是很高，所以所有访问
    // conf_的都使用这个锁也不会造成问题
    std::mutex   conf_lock_;
    ExecutorConf conf_;

    ExecutorConf get_executor_conf()override {
        PANIC("This function should not be called.");
        return conf_;
    }


    roo::ThreadPool executor_threads_;
    void executor_service_run(roo::ThreadObjPtr ptr);  // main task loop

public:

    int executor_start() {

        roo::log_info("about to start executor for host %s ... ", instance_name().c_str());
        executor_threads_.start_threads();
        return 0;
    }

    int executor_stop_graceful() {

        roo::log_info("about to stop executor for host %s ... ", instance_name().c_str());
        executor_threads_.graceful_stop_threads();

        return 0;
    }

    int executor_join() {

        roo::log_info("about to join executor for host %s ... ", instance_name().c_str());
        executor_threads_.join_threads();
        return 0;
    }

private:
    // 根据rpc_queue_自动伸缩线程负载
    void executor_threads_adjust(const boost::system::error_code& ec);
};

} // end namespace tzrpc


#endif // __RPC_EXECUTOR_H__
