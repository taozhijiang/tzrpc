#ifndef __RPC_EXECUTOR_H__
#define __RPC_EXECUTOR_H__

#include <xtra_rhel6.h>

#include <Utils/Log.h>
#include <Utils/EQueue.h>
#include <RPC/Service.h>

#include <Utils/ThreadPool.h>

namespace tzrpc {


class Executor: public Service {

public:

    Executor(std::shared_ptr<Service> service_impl, uint32_t thread_num, uint32_t thread_num_boost):
        service_impl_(service_impl),
        thread_num_(thread_num),
        thread_num_boost_(thread_num_boost),
        rpc_queue_() {
    }

    void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) override {
        rpc_queue_.PUSH(rpc_instance);
    }

    std::string instance_name() {
        return service_impl_->instance_name();
    }

    bool init() {

        if (!executor_threads_.init_threads(
            std::bind(&Executor::executor_service_run, this, std::placeholders::_1), thread_num_)) {
            log_err("executor_service_run init task failed!");
            return false;
        }

        return true;
    }

private:
    std::shared_ptr<Service> service_impl_;

    // 执行任务的线程组数目
    uint32_t thread_num_;
    uint32_t thread_num_boost_;

    EQueue<std::shared_ptr<RpcInstance>> rpc_queue_;

private:
    ThreadPool executor_threads_;
    void executor_service_run(ThreadObjPtr ptr);  // main task loop

public:

    int executor_start() {
        executor_threads_.start_threads();
        return 0;
    }

    int executor_stop_graceful() {

        log_err("about to stop executor... ");
        executor_threads_.graceful_stop_threads();

        return 0;
    }

    int executor_join() {
        executor_threads_.join_threads();
        return 0;
    }

};

} // end tzrpc


#endif // __RPC_EXECUTOR_H__
